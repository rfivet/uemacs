/* word.c -- implements word.h */
#include "word.h"

/* The routines in this file implement commands that work word or a
   paragraph at a time.  There are all sorts of word mode commands.  If I
   do any sentence mode commands, they are likely to be put in this file.

   Modified by Petri Kutvonen
 */

#include <assert.h>
#include <stdlib.h>	/* malloc, free */
#include <string.h> /* memcpy */

#include "basic.h"
#include "buffer.h"
#include "defines.h"
#include "isa.h"
#include "line.h"
#include "mlout.h"
#include "random.h"
#include "region.h"
#include "window.h"

#define ALLOCSZ	32
#define	TAB	0x09		/* a tab character              */

static int justflag = FALSE ;		/* justify, don't fill */

static int inword( void) ;

/* Word wrap on n-spaces.  Back-over whatever precedes the point on the
   current line and stop on the first word-break or the beginning of the
   line.  If we reach the beginning of the line, jump back to the end of
   the word and start a new line.  Otherwise, break the line at the
   word-break, eat it, and jump back to the end of the word.

   Returns TRUE on success, FALSE on errors.
 
   @f: default flag.
   @n: numeric argument.
 */
BINDABLE( wrapword) {
	int cnt;	/* size of word wrapped to next line */
	int c;		/* charector temporary */

	/* backup from the <NL> 1 char */
	if (!backchar(0, 1))
		return FALSE;

	/* back up until we aren't in a word,
	   make sure there is a break in the line */
	cnt = 0;
	while (((c = lgetc(curwp->w_dotp, curwp->w_doto)) != ' ')
	       && (c != '\t')) {
		cnt++;
		if (!backchar(0, 1))
			return FALSE;
		/* if we make it to the beginning, start a new line */
		if (curwp->w_doto == 0) {
			gotoeol(FALSE, 0);
			return lnewline();
		}
	}

	/* delete the forward white space */
	if (!forwdel(0, 1))
		return FALSE;

	/* put in a end of line */
	if (!lnewline())
		return FALSE;

	/* and past the first word */
	while (cnt-- > 0) {
		if (forwchar(FALSE, 1) == FALSE)
			return FALSE;
	}
	return TRUE;
}


/* Move the cursor backward by "n" words.  All of the details of motion are
   performed by the "backchar" and "forwchar" routines.  Error if you try
   to move beyond the buffers.
 */
BINDABLE( backword) {
	if( n < 0)
		return forwword( f, -n) ;

	if( backchar( FALSE, 1) == FALSE)
		return FALSE ;

	while( n--) {
		while( !inword())
			if( backchar( FALSE, 1) == FALSE)
				return FALSE ;

		do {
			if( backchar( FALSE, 1) == FALSE)
				return FALSE ;
		} while( inword()) ;
	}

	return forwchar( FALSE, 1) ;
}


/* Move the cursor forward by the specified number of words.  All of the
   motion is done by "forwchar".  Error if you try and move beyond the
   buffer's end.
 */
BINDABLE( forwword) {
	if( n < 0)
		return backword( f, -n) ;

	while( n--) {
		while( inword())
			if( forwchar( FALSE, 1) == FALSE)
				return FALSE ;

		do {
			if( forwchar( FALSE, 1) == FALSE)
				return FALSE ;
		} while( !inword()) ;
	}

	return TRUE ;
}

/* Word capitalize, to upper and to lower
*/
static boolean uniflip( boolean toupper_f) {	/* flip unicode case and forward */
	unicode_t	c ;
	int	len ;

	len = lgetchar( &c) ;	/* len => unicode or extended ASCII */
	if( (c <= 255) && ( toupper_f ? islower( c) : isupper( c))) {
		c = flipcase( c) ;
		ldelchar( 1, FALSE) ;
		if( len == 1)
			linsert_byte( 1, c) ;
		else
			linsert( 1, c) ;

		lchange( WFHARD) ;
	} else
		if( forwchar( FALSE, 1) == FALSE)
			return FALSE ;

	return TRUE ;
}

static boolean capcapword( int n, boolean first_f, boolean rest_f) {
	assert( !(curbp->b_mode & MDVIEW)) ;

	if( n < 0)
		return FALSE ;

	while( n--) {
		while( !inword())
			if( forwchar( FALSE, 1) == FALSE)
				return FALSE ;

		if( !uniflip( first_f))
			return FALSE ;

		while( inword())
			if( !uniflip( rest_f))
				return FALSE ;
	}

	return TRUE ;
}


/* Move the cursor forward by the specified number of words.  As you move,
   convert any characters to upper case.  Error if you try and move beyond
   the end of the buffer.  Bound to "M-U".
 */
BINDABLE( upperword) {
	return capcapword( n, TRUE, TRUE) ;
}


/* Move the cursor forward by the specified number of words.  As you move
   convert characters to lower case.  Error if you try and move over the
   end of the buffer.  Bound to "M-L".
 */
BINDABLE( lowerword) {
	return capcapword( n, FALSE, FALSE) ;
}


/* Move the cursor forward by the specified number of words.  As you move
   convert the first character of the word to upper case, and subsequent
   characters to lower case.  Error if you try and move past the end of the
   buffer.  Bound to "M-C".
 */
BINDABLE( capword) {
	return capcapword( n, TRUE, FALSE) ;
}


/* Kill forward by "n" words.  Remember the location of dot.  Move forward
   by the right number of words.  Put dot back where it was and issue the
   kill command for the right number of characters.  With a zero argument,
   just kill one word and no whitespace.  Bound to "M-D".
 */
BINDABLE( delfword) {
	line_p dotp;	/* original cursor line */
	int doto;	/*      and row */
	int c;		/* temp char */

	assert( !(curbp->b_mode & MDVIEW)) ;

	/* ignore the command if there is a negative argument */
	if (n < 0)
		return FALSE;

	/* Clear the kill buffer if last command wasn't a kill */
	if ((lastflag & CFKILL) == 0)
		kdelete();
	thisflag |= CFKILL;	/* this command is a kill */

	/* save the current cursor position */
	dotp = curwp->w_dotp;
	doto = curwp->w_doto;

	/* figure out how many characters to give the axe */
	long size = 0 ;

	/* get us into a word.... */
	while (inword() == FALSE) {
		if (forwchar(FALSE, 1) == FALSE)
			return FALSE;
		++size;
	}

	if (n == 0) {
		/* skip one word, no whitespace! */
		while (inword() == TRUE) {
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
			++size;
		}
	} else {
		/* skip n words.... */
		while (n--) {

			/* if we are at EOL; skip to the beginning of the next */
			while (curwp->w_doto == llength(curwp->w_dotp)) {
				if (forwchar(FALSE, 1) == FALSE)
					return FALSE;
				++size;
			}

			/* move forward till we are at the end of the word */
			while (inword() == TRUE) {
				if (forwchar(FALSE, 1) == FALSE)
					return FALSE;
				++size;
			}

			/* if there are more words, skip the interword stuff */
			if (n != 0)
				while (inword() == FALSE) {
					if (forwchar(FALSE, 1) == FALSE)
						return FALSE;
					++size;
				}
		}

		/* skip whitespace and newlines */
		while ((curwp->w_doto == llength(curwp->w_dotp)) ||
		       ((c = lgetc(curwp->w_dotp, curwp->w_doto)) == ' ')
		       || (c == '\t')) {
			if (forwchar(FALSE, 1) == FALSE)
				break;
			++size;
		}
	}

	/* restore the original position and delete the words */
	curwp->w_dotp = dotp;
	curwp->w_doto = doto;
	return ldelete(size, TRUE);
}


/* Kill backwards by "n" words.  Move backwards by the desired number of
   words, counting the characters.  When dot is finally moved to its
   resting place, fire off the kill command.  Bound to "M-Rubout" and to
   "M-Backspace".
 */
BINDABLE( delbword) {
	assert( !(curbp->b_mode & MDVIEW)) ;

	/* ignore the command if there is a nonpositive argument */
	if (n <= 0)
		return FALSE;

	/* Clear the kill buffer if last command wasn't a kill */
	if ((lastflag & CFKILL) == 0)
		kdelete();
	thisflag |= CFKILL;	/* this command is a kill */

	if (backchar(FALSE, 1) == FALSE)
		return FALSE;

	long size = 0 ;
	while (n--) {
		while (inword() == FALSE) {
			if (backchar(FALSE, 1) == FALSE)
				return FALSE;
			++size;
		}
		while (inword() != FALSE) {
			++size;
			if (backchar(FALSE, 1) == FALSE)
				goto bckdel;
		}
	}
	if (forwchar(FALSE, 1) == FALSE)
		return FALSE;
      bckdel:return ldelchar(size, TRUE);
}

/*
 * Return TRUE if the character at dot is a character that is considered to be
 * part of a word. The word character list is hard coded. Should be setable.
 */
static int inword( void) {
	int c;

	if( curwp->w_doto == llength( curwp->w_dotp))
		return FALSE ;

	c = lgetc( curwp->w_dotp, curwp->w_doto) ;
	return isletter( c) || ( c >= '0' && c <= '9') ;
}

static int parafillnjustify( int f, int n, int justify_f) {
	unicode_t c;		/* current char during scan	*/
	unicode_t *wbuf ;	/* buffer for current word	*/
	int wbufsize ;
	int wordlen;	/* length of current word       */
	int clength;	/* position on line during fill */
	int eopflag;	/* Are we at the End-Of-Paragraph? */
	int firstflag = TRUE ;	/* first word? (needs no space) */
	line_p eopline;	/* pointer to line just past EOP */
	int	dotflag = 0 ;		/* was the last char a period?  */
	int leftmarg = 0 ;		/* left marginal */

	assert( !(curbp->b_mode & MDVIEW)) ;

	if( fillcol == 0)	/* no fill column set */
		return mloutfail( "No fill column set") ;

	if( justify_f) {
		leftmarg = getccol( FALSE) ;
		if( leftmarg + 10 > fillcol)
			return mloutfail( "Column too narrow") ;

		justflag = justify_f ;
	}

	wbufsize = ALLOCSZ ;
	wbuf = malloc( ALLOCSZ * sizeof *wbuf) ;
	if( NULL == wbuf) {
		justflag = FALSE ;
		return FALSE ;
	}

	/* record the pointer to the line just past the EOP */
	gotoeop(FALSE, 1);
	eopline = lforw(curwp->w_dotp);

	/* and back to the beginning of the paragraph */
	gotobop(FALSE, 1);

	/* initialize various info */
	if( justflag && leftmarg < llength(curwp->w_dotp)) {
		setccol( leftmarg) ;
		lgetchar( &c) ;
		if( c == ' ' || c == '\t')
		/* on a space */
			if( getccol( TRUE) < getccol( FALSE))
			/* first non space before current position */
				firstflag = FALSE ;
	}

	clength = getccol( FALSE) ;
	wordlen = 0;

	/* scan through lines, filling words */
	eopflag = FALSE;
	while (!eopflag) {
		int bytes = 1;

		/* get the next character in the paragraph */
		if (curwp->w_doto == llength(curwp->w_dotp)) {
			c = ' ';
			if (lforw(curwp->w_dotp) == eopline)
				eopflag = TRUE;
		} else
			bytes = lgetchar(&c);

		/* and then delete it */
		ldelete(bytes, FALSE);

		/* if not a separator, just add it in */
		if (c != ' ' && c != '\t') {
			if (wordlen < wbufsize)
				wbuf[wordlen++] = c;
			else {
			/* overflow */
				unicode_t *newptr ;

				newptr = realloc( wbuf, (wbufsize + ALLOCSZ) * sizeof *wbuf) ;
				if( newptr != NULL) {
					wbuf = newptr ;
					wbufsize += ALLOCSZ ;
					wbuf[ wordlen++] = c ;
				} /* else the word is truncated silently */
			}
		} else if (wordlen) {
		/* at a word break with a word waiting */
			if( firstflag)
				firstflag = FALSE ;
			else {
			/* calculate tentative new length with word added */
				if( fillcol > clength + dotflag + 1 + wordlen) {
				/* add word to current line */
					linsert( dotflag + 1, ' ') ;	/* the space */
					clength += dotflag + 1 ;
				} else {
					/* start a new line */
					lnewline();
					linsert( leftmarg, ' ') ;
					clength = leftmarg;
				}
			}

			/* and add the word in in either case */
			for( int i = 0 ; i < wordlen ; i++) {
				c = wbuf[ i] ;
				linsert( 1, c) ;
				++clength;
			}

			dotflag = c == '.' ;	/* was the last char a period?  */
			wordlen = 0;
		}
	}

	/* and add a last newline for the end of our new paragraph */
	if( eopline == curbp->b_linep)	/* at EOF? */
		forwchar( FALSE, 1) ;
	else
		lnewline() ;

	if( justflag) {
		forwword(FALSE, 1);
		setccol( leftmarg) ;
		justflag = FALSE;
	}

	free( wbuf) ;
	return TRUE;
}


/* Fill the current paragraph according to the current
 * fill column
 *
 * f and n - deFault flag and Numeric argument
 */
BINDABLE( fillpara) {
	return parafillnjustify( f, n, FALSE) ;
}


/* Fill the current paragraph according to the current
 * fill column and cursor position
 *
 * int f, n;		deFault flag and Numeric argument
 */
BINDABLE( justpara) {
	return parafillnjustify( f, n, TRUE) ;
}


/* delete n paragraphs starting with the current one
 *
 * int f	default flag
 * int n	# of paras to delete
 */
BINDABLE( killpara) {
	while (n--) {		/* for each paragraph to delete */

		/* mark out the end and beginning of the para to delete */
		gotoeop(FALSE, 1);

		/* set the mark here */
		curwp->w_markp = curwp->w_dotp;
		curwp->w_marko = curwp->w_doto;

		/* go to the beginning of the paragraph */
		gotobop(FALSE, 1);
		curwp->w_doto = 0;	/* force us to the beginning of line */

		/* and delete it */
		int status = killregion( FALSE, 1) ;
		if( status != TRUE)
			return status ;

		/* and clean up the 2 extra lines */
		ldelete(2L, TRUE);
	}

	return TRUE ;
}


/*	wordcount:	count the # of words in the marked region,
 *			along with average word sizes, # of chars, etc,
 *			and report on them.
 *
 * int f, n;		ignored numeric arguments
 */
BINDABLE( wordcount) {
	line_p lp;	/* current line to scan */
	int offset;	/* current char to scan */
	long size;		/* size of region left to count */
	int ch;	/* current character to scan */
	int wordflag;	/* are we in a word now? */
	int lastword;	/* were we just in a word? */
	long nwords;		/* total # of words */
	long nchars;		/* total number of chars */
	int nlines;		/* total number of lines in region */
	int avgch;		/* average number of chars/word */
	int status;		/* status return code */
	region_t region ;	/* region to look at */

	/* make sure we have a region to count */
	if( (status = getregion( &region)) != TRUE)
		return status;
	lp = region.r_linep;
	offset = region.r_offset;
	size = region.r_size;

	/* count up things */
	lastword = FALSE;
	nchars = 0L;
	nwords = 0L;
	nlines = 0;
	while (size--) {

		/* get the current character */
		if (offset == llength(lp)) {	/* end of line */
			ch = '\n';
			lp = lforw(lp);
			offset = 0;
			++nlines;
		} else {
			ch = lgetc(lp, offset);
			++offset;
		}

		/* and tabulate it */
		wordflag = isletter( ch) || (ch >= '0' && ch <= '9') ;
		if (wordflag == TRUE && lastword == FALSE)
			++nwords;
		lastword = wordflag;
		++nchars;
	}

	/* and report on the info */
	if (nwords > 0L)
		avgch = (int) ((100L * nchars) / nwords);
	else
		avgch = 0;

	mloutfmt( "Words %D Chars %D Lines %d Avg chars/word %f",
		nwords, nchars, nlines + 1, avgch) ;
	return TRUE;
}


/* go back to the beginning of the current paragraph
 * here we look for a <NL><NL> or <NL><TAB> or <NL><SPACE>
 * combination to delimit the beginning of a paragraph
 *
 * int f, n;		default Flag & Numeric argument
 */
BINDABLE( gotobop) {
	if (n < 0) /* the other way... */
		return gotoeop(f, -n);

	while (n-- > 0) {  /* for each one asked for */

		/* first scan back until we are in a word */
		while( backchar( FALSE, 1) && !inword()) ;

		curwp->w_doto = 0;	/* and go to the B-O-Line */

		/* and scan back until we hit a <NL><NL> or <NL><TAB>
		   or a <NL><SPACE>                                     */
		while (lback(curwp->w_dotp) != curbp->b_linep)
			if (llength(curwp->w_dotp) != 0 &&
			    ((justflag == TRUE) ||
			     (lgetc(curwp->w_dotp, curwp->w_doto) != TAB &&
			      lgetc(curwp->w_dotp, curwp->w_doto) != ' '))
			    )
				curwp->w_dotp = lback(curwp->w_dotp);
			else
				break;

		/* and then forward until we are in a word */
		while( !inword() && forwchar( FALSE, 1)) ;
	}
	curwp->w_flag |= WFMOVE;	/* force screen update */
	return TRUE;
}


/* Go forward to the end of the current paragraph here we look for a
   <NL><NL> or <NL><TAB> or <NL><SPACE> combination to delimit the
   beginning of a paragraph

   int f, n;		default Flag & Numeric argument
 */
BINDABLE( gotoeop) {
	if (n < 0)  /* the other way... */
		return gotobop(f, -n);

	while (n-- > 0) {  /* for each one asked for */
		/* first scan forward until we are in a word */
		while( !inword() && forwchar( FALSE, 1)) ;
		curwp->w_doto = 0 ;						/* and go to the B-O-Line */
		if( curwp->w_dotp != curbp->b_linep)	/* of next line if not at EOF */
			curwp->w_dotp = lforw( curwp->w_dotp) ;

		/* and scan forward until we hit a <NL><NL> or <NL><TAB>
		   or a <NL><SPACE>                                     */
		while (curwp->w_dotp != curbp->b_linep) {
			if (llength(curwp->w_dotp) != 0 &&
			    ((justflag == TRUE) ||
			     (lgetc(curwp->w_dotp, curwp->w_doto) != TAB &&
			      lgetc(curwp->w_dotp, curwp->w_doto) != ' '))
			    )
				curwp->w_dotp = lforw(curwp->w_dotp);
			else
				break;
		}

		/* and then backward until we are in a word */
		while( backchar( FALSE, 1) && !inword()) ;

		curwp->w_doto = llength(curwp->w_dotp);	/* and to the EOL */
	}
	curwp->w_flag |= WFMOVE;  /* force screen update */
	return TRUE;
}

/* end of word.c */
