/* line.c -- implements line.h */
#include "line.h"

/* The functions in this file are a general set of line management utilities.
 * They are the only routines that touch the text. They also touch the buffer
 * and window structures, to make sure that the necessary updating gets done.
 * There are routines in this file that handle the kill buffer too. It isn't
 * here for any good reason.
 *
 * Note that this code only updates the dot and mark values in the window list.
 * Since all the code acts on the current window, the buffer that we are
 * editing must be being displayed, which means that "b_nwnd" is non zero,
 * which means that the dot and mark values in the buffer headers are nonsense.
 *
 */

#include <assert.h>
#include <stddef.h> /* NULL, offsetof() */
#include <stdlib.h> /* malloc(), free() */
#include <string.h>

#include "buffer.h"
#include "defines.h"
#include "list.h"
#include "mlout.h"
#include "utf8.h"
#include "window.h"


int tabwidth = 8 ;          /* column span of a tab */

static int ldelnewline( void) ;

/* The editor holds deleted text chunks in the struct kill buffer. The
 * kill buffer is logically a stream of ascii characters, however
 * due to its unpredicatable size, it gets implemented as a linked
 * list of chunks. (The d_ prefix is for "deleted" text, as k_
 * was taken up by the keycode structure).
 */

#define KBLOCK  248				/* sizeof kill buffer chunks    */

typedef struct kill {
    struct kill *d_next ;   	/* Link to next chunk, NULL if last. */
    char d_chunk[ KBLOCK] ;		/* Deleted text. */
} *kill_p ;

static kill_p kbufp = NULL ;    /* current kill buffer chunk pointer */
static kill_p kbufh = NULL ;    /* kill buffer header pointer */
static int kused = KBLOCK ;     /* # of bytes used in kill buffer */
static int klen ;               /* length of kill buffer content */
static char *value = NULL ;     /* temp buffer for value */

/*
 * return some of the contents of the kill buffer
 */
const char *getkill( void) {
/* no kill buffer or no memory .... just return a null string */
    if( kbufh == NULL
	|| (value = realloc( value, klen + 1)) == NULL)
        return "" ;

    char *cp = value ;
    for( kill_p kp = kbufh ; kp != NULL ; kp = kp->d_next) {
        int size = (kp->d_next != NULL) ? KBLOCK : kused ;
        memcpy( cp, kp->d_chunk, size) ;
        cp += size ;
    }

    *cp = 0 ;

/* and return the constructed value */
    return value ;
}


/* Move the cursor backwards by "n" combined characters.  If "n" is less
   than zero call "forwchar" to actually do the move.  Otherwise compute
   the new cursor location.  Error if you try and move out of the buffer.
   Set the flag if the line pointer for dot changes.
 */
BBINDABLE( backchar) {
    assert( f == TRUE || n == 1) ;
    if( n < 0)
        return forwchar( f, -n) ;

    while( n--) {
        if( curwp->w_doto == 0) {   /* at beginning of line */
            line_p lp = lback( curwp->w_dotp) ;
            if( lp == curbp->b_linep)   /* at beginning of buffer */
                return FALSE ;

            curwp->w_dotp = lp ;
            curwp->w_doto = llength( lp) ;
            curwp->w_flag |= WFMOVE ;
        } else {
			unsigned pos ;
		/* move back over combining unicode */
		combined:
            pos = curwp->w_doto -= 1 ;
		/* check if at end of unicode */
            if( pos > 0) {
                unsigned delta = utf8_revdelta(
					(unsigned char *) &( (curwp->w_dotp)->l_text[ pos]), pos) ;
				if( delta != 0) {
	                pos = curwp->w_doto -= delta ;
					if( pos > 0) {	/* check if on combining unicode */
						unicode_t unc ;
				
	        	        utf8_to_unicode( curwp->w_dotp->l_text, pos,
												llength( curwp->w_dotp), &unc) ;
		        	    if( utf8_width( unc) == 0)
							goto combined ;
					}
				}
			}
        }
    }

    return TRUE ;
}


/* Move the cursor forwards by "n" combined characters.  If "n" is less
   than zero call "backchar" to actually do the move.  Otherwise compute
   the new cursor location, and move ".".  Error if you try and move off
   the end of the buffer.  Set the flag if the line pointer for dot
   changes.
 */
BBINDABLE( forwchar) {
    assert( f == TRUE || n == 1) ;
    if( n < 0)
        return backchar( f, -n) ;

    while( n--) {
        int len = llength( curwp->w_dotp) ;
        if( curwp->w_doto == len) { /* at end of line */
            if( curwp->w_dotp == curbp->b_linep)    /* at end of buffer */
                return FALSE ;

            curwp->w_dotp = lforw( curwp->w_dotp) ;
            curwp->w_doto = 0 ;
            curwp->w_flag |= WFMOVE ;
        } else {
            unicode_t unc ;

            curwp->w_doto += utf8_to_unicode( curwp->w_dotp->l_text,
                                                    curwp->w_doto, len, &unc) ;
        /* check if next char is null width unicode */
            while( curwp->w_doto < len - 1) {
                unsigned bytes = utf8_to_unicode( curwp->w_dotp->l_text,
                                                    curwp->w_doto, len, &unc) ;
                if( utf8_width( unc) == 0)
                    curwp->w_doto += bytes ;
                else
                    break ;
            }
        }
    }

    return TRUE ;
}


/* This routine allocates a block of memory large enough to hold a struct line
 * containing "used" characters. The block is always rounded up a bit. Return
 * a pointer to the new block, or NULL if there isn't any memory left. Print a
 * message in the message line if no space.
 */
line_p lalloc( int used) {
#define BLOCK_SIZE 16   /* Line block chunk size. */

/* rounding down use masking instead of modulo when BLOCK_SIZE is power of 2 */
#if (BLOCK_SIZE & -BLOCK_SIZE) == BLOCK_SIZE
    int size = (used + BLOCK_SIZE) & ~(BLOCK_SIZE - 1) ;
#else
    int size = used + BLOCK_SIZE - used % BLOCK_SIZE ;
#endif
    line_p lp = malloc( sizeof *lp + size) ;
    if( lp == NULL)
        mloutstr( "(OUT OF MEMORY)") ;
    else {
        lp->l_size = size ;
        lp->l_used = used ;
    }

    return lp ;
}


/* Delete line "lp". Fix all of the links that might point at it (they are
 * moved to offset 0 of the next line. Unlink the line from whatever buffer it
 * might be in. Release the memory. The buffers are updated too; the magic
 * conditions described in the above comments don't hold here.
 */
void lfree( line_p lp) {
    for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
        if( wp->w_linep == lp)
            wp->w_linep = lp->l_fp ;

        if( wp->w_dotp == lp) {
            wp->w_dotp = lp->l_fp ;
            wp->w_doto = 0 ;
        }

        if( wp->w_markp == lp) {
            wp->w_markp = lp->l_fp ;
            wp->w_marko = 0 ;
        }
    }

    for( buffer_p bp = bheadp ; bp != NULL ; bp = bp->b_bufp) {
        if( bp->b_nwnd == 0) {
            if( bp->b_dotp == lp) {
                bp->b_dotp = lp->l_fp ;
                bp->b_doto = 0 ;
            }
			
            if( bp->b_markp == lp) {
                bp->b_markp = lp->l_fp ;
                bp->b_marko = 0 ;
            }
        }
    }

    lp->l_bp->l_fp = lp->l_fp ;
    lp->l_fp->l_bp = lp->l_bp ;
    free( lp) ;
}


/* This routine gets called when a character is changed in place in the current
 * buffer. It updates all of the required flags in the buffer and window
 * system. The flag used is passed as an argument; if the buffer is being
 * displayed in more than 1 window we change EDIT to HARD. Set MODE if the
 * mode line needs to be updated (the "*" has to be set).
 */
void lchange( int flag) {
    if( curbp->b_nwnd != 1) /* Ensure hard.     */
        flag = WFHARD ;

    if( (curbp->b_flag & BFCHG) == 0) { /* First change, so     */
        flag |= WFMODE ; /* update mode lines.   */
        curbp->b_flag |= BFCHG ;
    }

    for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp)
        if( wp->w_bufp == curbp)
            wp->w_flag |= flag ;
}


/* insert spaces forward into text
 *
 * int f, n;        default flag and numeric argument
 */
BINDABLE( insspace) {
    assert( !(curbp->b_mode & MDVIEW)) ;
    linsert( n, ' ') ;
    backchar( f, n) ;
    return TRUE ;
}


/* linstr -- Insert a string at the current point */
boolean linstr( char *instr) {
    boolean status = TRUE ;
    if( instr != NULL) {
        int c ;

        while( (c = (unsigned char) *instr++)) {
            status = (c == '\n') ? lnewline() : linsert_byte( 1, c) ;
            if( status != TRUE) {	/* Insertion error? */
                mloutstr( "%Out of memory while inserting") ;
                break ;
            }
        }
    }

    return status ;
}

/*
 * Insert "n" copies of the character "c" at the current location of dot. In
 * the easy case all that happens is the text is stored in the line. In the
 * hard case, the line has to be reallocated. When the window list is updated,
 * take special care; I screwed it up once. You always update dot in the
 * current window. You update mark, and a dot in another window, if it is
 * greater than the place where you did the insert. Return TRUE if all is
 * well, and FALSE on errors.
 */

boolean linsert_byte( int n, int c) {
    char *cp1;
    char *cp2;
    line_p lp2, lp3 ;
	int i ;

    assert( (curbp->b_mode & MDVIEW) == 0) ;

    lchange( WFEDIT) ;
    line_p lp1 = curwp->w_dotp ;	/* Current line         */
    if( lp1 == curbp->b_linep) {    /* At the end: special  */
        if( curwp->w_doto != 0)
            return mloutfail( "bug: linsert") ;

        lp2 = lalloc( n) ;  /* Allocate new line */
        if( lp2 == NULL)
            return FALSE ;

        lp3 = lp1->l_bp ;	/* Previous line        */
        lp3->l_fp = lp2 ;	/* Link in              */
        lp2->l_fp = lp1 ;
        lp1->l_bp = lp2 ;
        lp2->l_bp = lp3 ;
        for( i = 0 ; i < n ; ++i)
            lp2->l_text[ i] = c ;

        curwp->w_dotp = lp2 ;
        curwp->w_doto = n ;
        return TRUE ;
    }

    int doto = curwp->w_doto ;	/* Save for later.      */
    if( lp1->l_used + n > lp1->l_size) {    /* Hard: reallocate     */
        lp2 = lalloc( lp1->l_used + n) ;
        if( lp2 == NULL)
            return FALSE ;

        cp1 = &lp1->l_text[ 0] ;
        cp2 = &lp2->l_text[ 0] ;
        while( cp1 != &lp1->l_text[ doto])
            *cp2++ = *cp1++ ;

        cp2 += n ;
        while( cp1 != &lp1->l_text[ lp1->l_used])
            *cp2++ = *cp1++ ;

        lp1->l_bp->l_fp = lp2 ;
        lp2->l_fp = lp1->l_fp ;
        lp1->l_fp->l_bp = lp2 ;
        lp2->l_bp = lp1->l_bp ;
        free( lp1) ;
    } else {        /* Easy: in place       */
        lp2 = lp1 ; /* Pretend new line     */
        lp2->l_used += n ;
        cp2 = &lp1->l_text[ lp1->l_used] ;
        cp1 = cp2 - n ;
        while( cp1 != &lp1->l_text[ doto])
            *--cp2 = *--cp1 ;
    }

    for( i = 0 ; i < n ; ++i) /* Add the characters       */
        lp2->l_text[ doto + i] = c ;

/* Update windows */
    for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
        if( wp->w_linep == lp1)
            wp->w_linep = lp2 ;

        if( wp->w_dotp == lp1) {
            wp->w_dotp = lp2 ;
            if( wp == curwp || wp->w_doto > doto)
                wp->w_doto += n ;
        }

        if( wp->w_markp == lp1) {
            wp->w_markp = lp2 ;
            if( wp->w_marko > doto)
                wp->w_marko += n ;
        }
    }

    return TRUE ;
}

boolean linsert( int n, unicode_t c) {
    assert( n >= 0) ;
    assert( !(curbp->b_mode & MDVIEW)) ;

    if( n > 0) {
        char utf8[ 4] ;
        int bytes ;

        bytes = unicode_to_utf8(c, utf8) ;
        if (bytes == 1)
            return linsert_byte(n, (unsigned char) utf8[0]);

        do {
            int j ;

            for( j = 0 ; j < bytes ; j += 1)
                if( !linsert_byte( 1, (unsigned char) utf8[ j]))
                    return FALSE ;
        } while( --n > 0) ;
    }

    return TRUE;
}

/* Overwrite a character into the current line at the current position
 *
 * int c ;  character to overwrite on current position
 */
static boolean lowrite( int c) {
    if( curwp->w_doto < curwp->w_dotp->l_used
    && (    lgetc( curwp->w_dotp, curwp->w_doto) != '\t'
        ||  (curwp->w_doto % tabwidth) == (tabwidth - 1)
    ))
        ldelchar( 1, FALSE) ;

    return linsert( 1, c) ;
}


/* lover -- Overwrite a string at the current point */
boolean lover( char *ostr) {
    boolean status = TRUE ;
    if( ostr != NULL) {
        int c ;

        while( (c = (unsigned char) *ostr++)) {
            status = (c == '\n') ? lnewline() : lowrite( c) ;
            if( status != TRUE) {   /* Insertion error? */
                mloutstr( "%Out of memory while overwriting") ;
                break ;
            }
        }
    }

    return status ;
}


/* Insert a newline into the buffer at the current location of dot in the
   current window.  The funny ass-backwards way it does things is not a
   botch; it just makes the last line in the file not a special case.
   Return TRUE if everything works out and FALSE on error (memory
   allocation failure).  The update of dot and mark is a bit easier then in
   the above case, because the split forces more updating.
 */
boolean lnewline( void) {
    assert( !(curbp->b_mode & MDVIEW)) ;

#if SCROLLCODE
    lchange(WFHARD | WFINS);
#else
    lchange(WFHARD);
#endif
    line_p lp1 = curwp->w_dotp ;	/* Get the address and	*/
    int doto = curwp->w_doto ;		/* offset of "."        */
    line_p lp2 = lalloc( doto) ;	/* New first half line	*/
    if( lp2 == NULL)
        return FALSE ;

    memcpy( lp2->l_text, lp1->l_text, doto) ;
    lp1->l_used -= doto ;
	memcpy( lp1->l_text, &lp1->l_text[ doto], lp1->l_used) ;
    lp2->l_fp = lp1 ;
    lp2->l_bp = lp1->l_bp ;
    lp1->l_bp = lp2 ;
    lp2->l_bp->l_fp = lp2 ;
    for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
        if( wp->w_linep == lp1)
            wp->w_linep = lp2 ;

        if( wp->w_dotp == lp1) {
            if( wp->w_doto < doto)
                wp->w_dotp = lp2 ;
            else
                wp->w_doto -= doto ;
        }

        if (wp->w_markp == lp1) {
            if( wp->w_marko < doto)
                wp->w_markp = lp2 ;
            else
                wp->w_marko -= doto ;
        }
    }

    return TRUE ;
}


/* lgetchar():
 *  get unicode value and return UTF-8 size of character at dot.
 */
int lgetchar( unicode_t *cp) {
    if( curwp->w_dotp->l_used == curwp->w_doto) {		/* at EOL? */
        *cp = (curbp->b_mode & MDDOS) ? '\r' : '\n' ;
        return 1 ;
    } else
        return utf8_to_unicode( curwp->w_dotp->l_text, curwp->w_doto,
                                                llength( curwp->w_dotp), cp) ;
}


/* lcombinedsize():
 *  return total UTF-8 size of combined character at dot.
 */
static int lcombinedsize( void) {
    if( curwp->w_dotp->l_used == curwp->w_doto)	/* EOL? */
        return 1 ;
    else {
		unicode_t c ;

		int pos = curwp->w_doto ;
        unsigned bytes = utf8_to_unicode( curwp->w_dotp->l_text, pos,
                                                llength( curwp->w_dotp), &c) ;
	/* check if followed by combining unicode character */
		pos += bytes ;
		while( pos < llength( curwp->w_dotp) - 1) {		/* at least 2 bytes */
			unsigned cnt = utf8_to_unicode( curwp->w_dotp->l_text, pos,
                                                llength( curwp->w_dotp), &c) ;
			if( utf8_width( c) == 0) {
				bytes += cnt ;
				pos += cnt ;
			} else
				break ;
		}
		
		return bytes ;
	}
}


/* ldelchar():
 *  delete forward combined characters starting at dot.
 *
 * ldelete() really fundamentally works on bytes, not characters.
 * It is used for things like "scan 5 words forwards, and remove
 * the bytes we scanned".
 *
 * If you want to delete characters, use ldelchar().
 */
boolean ldelchar( long n, boolean kill_f) {
/* testing for read only mode is done by ldelete() */
    while( n-- > 0)
        if( !ldelete( lcombinedsize(), kill_f))
            return FALSE ;

    return TRUE ;
}


/* This function deletes "n" bytes, starting at dot. It understands how do deal
 * with end of lines, etc. It returns TRUE if all of the characters were
 * deleted, and FALSE if they were not (because dot ran into the end of the
 * buffer. The "kflag" is TRUE if the text should be put in the kill buffer.
 *
 * long n;      # of chars to delete
 * int kflag;        put killed text in kill buffer flag
 */
boolean ldelete( long n, boolean kflag) {
    assert( !(curbp->b_mode & MDVIEW)) ;

    while( n > 0) {
        line_p dotp = curwp->w_dotp ;
        if( dotp == curbp->b_linep)	/* Hit end of buffer.       */
            return FALSE ;

        int doto = curwp->w_doto ;
        int chunk = dotp->l_used - doto ;	/* Size of chunk.       */
        if( chunk == 0) {   /* End of line, merge.  */
#if SCROLLCODE
            lchange( WFHARD | WFKILLS) ;
#else
            lchange( WFHARD) ;
#endif
            if( ldelnewline() == FALSE
            || (kflag != FALSE && kinsert( '\n') == FALSE))
                return FALSE ;

            --n ;
            continue ;
        } else if( chunk > n)
            chunk = n ;

        lchange( WFEDIT) ;
        char *cp1 = &dotp->l_text[ doto] ;	/* Scrunch text.        */
        char *cp2 = cp1 + chunk ;
        if( kflag != FALSE) {   /* Kill?                */
            while( cp1 != cp2) {
                if( kinsert( *cp1) == FALSE)
                    return FALSE ;
					
                ++cp1 ;
            }

            cp1 = &dotp->l_text[ doto] ;
        }

        while( cp2 != &dotp->l_text[ dotp->l_used])
            *cp1++ = *cp2++ ;

        dotp->l_used -= chunk ;

	/* Fix windows */
        for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
            if( wp->w_dotp == dotp && wp->w_doto >= doto) {
                wp->w_doto -= chunk ;
                if( wp->w_doto < doto)
                    wp->w_doto = doto ;
            }

            if( wp->w_markp == dotp && wp->w_marko >= doto) {
                wp->w_marko -= chunk ;
                if( wp->w_marko < doto)
                    wp->w_marko = doto ;
            }
        }

        n -= chunk ;
    }

    return TRUE ;
}


/* Delete a newline. Join the current line with the next line. If the next line
 * is the magic header line always return TRUE; merging the last line with the
 * header line can be thought of as always being a successful operation, even
 * if nothing is done, and this makes the kill buffer work "right". Easy cases
 * can be done by shuffling data around. Hard cases require that lines be moved
 * about in memory. Return FALSE on error and TRUE if all looks ok. Called by
 * "ldelete" only.
 */
static int ldelnewline( void) {
    char *cp1;
    char *cp2;
	window_p wp ;

    assert( (curbp->b_mode & MDVIEW) == 0) ;

    line_p lp1 = curwp->w_dotp ;
    line_p lp2 = lp1->l_fp ;
    if( lp2 == curbp->b_linep) {    /* At the buffer end.   */
        if( lp1->l_used == 0)   /* Blank line.              */
            lfree( lp1) ;

        return TRUE ;
    }
	
    if( lp2->l_used <= lp1->l_size - lp1->l_used) {
        cp1 = &lp1->l_text[ lp1->l_used] ;
        cp2 = lp2->l_text ;
        while( cp2 != &lp2->l_text[ lp2->l_used])
            *cp1++ = *cp2++ ;

        for( wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
            if( wp->w_linep == lp2)
                wp->w_linep = lp1 ;

            if( wp->w_dotp == lp2) {
                wp->w_dotp = lp1 ;
                wp->w_doto += lp1->l_used ;
            }

            if( wp->w_markp == lp2) {
                wp->w_markp = lp1 ;
                wp->w_marko += lp1->l_used ;
            }
        }

        lp1->l_used += lp2->l_used ;
        lp1->l_fp = lp2->l_fp ;
        lp2->l_fp->l_bp = lp1 ;
        free( lp2) ;
        return TRUE ;
    }

    line_p lp3 = lalloc( lp1->l_used + lp2->l_used) ;
    if( lp3 == NULL)
        return FALSE ;

    cp1 = lp1->l_text ;
    cp2 = lp3->l_text ;
    while( cp1 != &lp1->l_text[ lp1->l_used])
        *cp2++ = *cp1++ ;

    cp1 = lp2->l_text ;
    while( cp1 != &lp2->l_text[ lp2->l_used])
        *cp2++ = *cp1++ ;

    lp1->l_bp->l_fp = lp3 ;
    lp3->l_fp = lp2->l_fp ;
    lp2->l_fp->l_bp = lp3 ;
    lp3->l_bp = lp1->l_bp ;
    for( wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
        if( wp->w_linep == lp1 || wp->w_linep == lp2)
            wp->w_linep = lp3 ;

        if( wp->w_dotp == lp1)
            wp->w_dotp = lp3 ;
        else if( wp->w_dotp == lp2) {
            wp->w_dotp = lp3 ;
            wp->w_doto += lp1->l_used ;
        }

        if( wp->w_markp == lp1)
            wp->w_markp = lp3 ;
        else if( wp->w_markp == lp2) {
            wp->w_markp = lp3 ;
            wp->w_marko += lp1->l_used ;
        }
    }

    free( lp1) ;
    free( lp2) ;
    return TRUE ;
}


/* Delete all of the text saved in the kill buffer. Called by commands when a
 * new kill context is being created. The kill buffer array is released, just
 * in case the buffer has grown to immense size. No errors.
 */
void kdelete( void) {
    if( kbufh != NULL) {
    /* first, delete all the chunks */
		freelist( (list_p) kbufh) ;

    /* and reset all the kill buffer pointers */
        kbufh = kbufp = NULL ;
        kused = KBLOCK ;
        klen = 0 ;
        if( value != NULL) {
            free( value) ;
            value = NULL ;
        }
    }
}

/*
 * Insert a character to the kill buffer, allocating new chunks as needed.
 * Return TRUE if all is well, and FALSE on errors.
 *
 * int c;           character to insert in the kill buffer
 */
int kinsert( int c) {
    /* check to see if we need a new chunk */
    if( kused >= KBLOCK) {
        kill_p nchunk = malloc( sizeof *nchunk) ;
        if( nchunk == NULL)
            return FALSE ;

        if( kbufh == NULL) {    /* set head ptr if first time */
            kbufh = nchunk ;
            klen = 0 ;
        }

        if( kbufp != NULL)  /* point the current to this new one */
            kbufp->d_next = nchunk ;

        kbufp = nchunk ;
        kbufp->d_next = NULL ;
        kused = 0 ;
    }

    /* and now insert the character */
    kbufp->d_chunk[ kused++] = c ;
    klen += 1 ;
    return TRUE ;
}

/*
 * Yank text back from the kill buffer. This is really easy. All of the work
 * is done by the standard insert routines. All you do is run the loop, and
 * check for errors. Bound to "C-Y".
 */
BINDABLE( yank) {
    int c;
    int i;
    char *sp;   /* pointer into string to insert */
    kill_p kp;      /* pointer into kill buffer */

    assert( !(curbp->b_mode & MDVIEW)) ;

    if (n < 0)
        return FALSE;
    /* make sure there is something to yank */
    if (kbufh == NULL)
        return TRUE;    /* not an error, just nothing */

    /* for each time.... */
    while (n--) {
        kp = kbufh;
        while (kp != NULL) {
            if (kp->d_next == NULL)
                i = kused;
            else
                i = KBLOCK;
            sp = kp->d_chunk;
            while (i--) {
                if ((c = *sp++) == '\n') {
                    if (lnewline() == FALSE)
                        return FALSE;
                } else {
                    if (linsert_byte(1, c) == FALSE)
                        return FALSE;
                }
            }
            kp = kp->d_next;
        }
    }
    return TRUE;
}

/*
 * tell the user that this command is illegal while we are in
 * VIEW (read-only) mode
 */
boolean rdonly( void) {
    return mloutfail( "(Key illegal in VIEW mode)") ;
}

/* end of line.c */
