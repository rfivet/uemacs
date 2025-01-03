/* file.c -- implements file.h */
#include "file.h"

/* The routines in this file handle the reading, writing and lookup of disk
   files.  All of details about the reading and writing of the disk are in
   "fileio.c".

   modified by Petri Kutvonen
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "defines.h"
#include "display.h"
#include "execute.h"
#include "fileio.h"
#include "input.h"
#include "line.h"
#include "lock.h"
#include "mlout.h"
#include "util.h"
#include "utf8.h"
#include "window.h"

typedef enum {
    EOL_NONE,
    EOL_UNIX,
    EOL_DOS,
    EOL_MAC,
    EOL_MIXED
} eoltype ;

static const char *eolname[] = {
    "NONE",
    "UNIX",
    "DOS",
    "MAC",
    "MIXED"
} ;

static const char *codename[] = {
    "ASCII",
    "UTF-8",
    "EXTENDED",
    "MIXED"
} ;

boolean restflag = FALSE ;  /* restricted use?              */


static int ifile( const char *fname) ;


boolean resterr( void) {
    mloutfmt( "%B(That command is RESTRICTED)") ;
    return FALSE ;
}


/* Read a file into the current buffer.  This is really easy; all you do is
 * find the name of the file, and call the standard "read a file into the
 * current buffer" code.  Bound to C-X C-R read-file.
 */
BINDABLE( fileread) {
    char *fname ;

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    int status = newmlarg( &fname, "read-file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
        status = readin( fname, TRUE) ;
        free( fname) ;
    }

    return status ;
}


/* Insert a file into the current buffer.  This is really easy; all you do
 * is find the name of the file, and call the standard "insert a file into
 * the current buffer" code.  Bound to C-X C-I insert-file.
 */
BINDABLE( insfile) {
    char *fname ;

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    assert( !(curbp->b_mode & MDVIEW)) ;

    int status = newmlarg( &fname, "insert-file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
        status = ifile( fname) ;
        free( fname) ;
    }

    if( status != TRUE)
        return status ;

    return reposition( TRUE, -1) ;  /* Redraw with dot at bottom of window */
}


/* Select a file for editing.  Look around to see if you can find the file
 * in another buffer; if you can find it just switch to the buffer.  If you
 * cannot find the file, create a new buffer, read in the text, and switch
 * to the new buffer.  Bound to C-X C-F find-file.
 */
BINDABLE( filefind) {
    char *fname ;   /* file user wishes to find */

    if( restflag)   /* don't allow this command if restricted */
        return resterr() ;

    int status = newmlarg( &fname, "find-file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
        status = getfile( fname, TRUE) ;
        free( fname) ;
    }

    return status ;
}


/* Update mode lines of all windows displaying current buffer */
static void upd_mode( void) {
    for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp)
        if( wp->w_bufp == curbp)
            wp->w_flag |= WFMODE ;
}


BINDABLE( viewfile) {   /* visit a file in VIEW mode */
    char *fname ;   /* file user wishes to find */

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    int status = newmlarg( &fname, "view-file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
        status = getfile( fname, FALSE) ;
        free( fname) ;

        if( status == TRUE) {   /* if we succeed, put it in view mode */
            curwp->w_bufp->b_mode |= MDVIEW ;
            upd_mode() ;
        }
    }

    return status ;
}


/* getfile()
 *
 * char fname[];    file name to find
 * boolean lockfl;      check the file for locks?
 */
int getfile( const char *fname, boolean lockfl) {
    buffer_p bp;
    int s;
    bname_t bname ;  /* buffer name to put file */

/* Is the file in current buffer? */
    if( strcmp( curbp->b_fname, fname) == 0) {
        mloutstr( "(Current buffer)") ;
        return TRUE ;
    }

/* Is the file in any buffer? */
    for( bp = bheadp; bp != NULL; bp = bp->b_bufp) {
        if( (bp->b_flag & BFINVS) == 0
        && strcmp( bp->b_fname, fname) == 0) {
            line_p  lp ;
            int i ;

            swbuffer( bp) ;

        /* Center dotted line in window */
            i = curwp->w_ntrows / 2 ;
            for( lp = curwp->w_dotp ; lback( lp) != curbp->b_linep ; lp = lback( lp))
                if( i-- == 0)
                    break ;

            curwp->w_linep = lp ;

        /* Refresh window */
            curwp->w_flag |= WFMODE | WFHARD ;
            cknewwindow() ;
            mloutstr( "(Old buffer)") ;
            return TRUE ;
        }
    }

    makename(bname, fname); /* New buffer name.     */
    while ((bp = bfind(bname, FALSE, 0)) != NULL) {
        char *new_bname ;

        /* old buffer name conflict code */
        s = newmlarg( &new_bname, "Buffer name: ", sizeof( bname_t)) ;
        if( s == ABORT) /* ^G to just quit      */
            return s ;
        else if (s == FALSE) {   /* CR to clobber it     */
            makename( bname, fname) ;
            break ;
        } else { /* TRUE */
            mystrscpy( bname, new_bname, sizeof bname) ;
            free( new_bname) ;
        }
    }

    if (bp == NULL && (bp = bfind(bname, TRUE, 0)) == NULL) {
        mloutstr( "Cannot create buffer") ;
        return FALSE;
    }
    if (--curbp->b_nwnd == 0) { /* Undisplay.           */
        curbp->b_dotp = curwp->w_dotp;
        curbp->b_doto = curwp->w_doto;
        curbp->b_markp = curwp->w_markp;
        curbp->b_marko = curwp->w_marko;
    }
    curbp = bp;     /* Switch to it.        */
    curwp->w_bufp = bp;
    curbp->b_nwnd++;
    s = readin(fname, lockfl);  /* Read it in.          */
    cknewwindow();
    return s;
}


/* Read file "fname" into the current buffer, blowing away any text found
   there.  Called by both the read and find commands.  Return the final
   status of the read.  Also called by the mainline, to read in a file
   specified on the command line as an argument.  The command bound to
   M-FNR is called after the buffer is set up and before it is read.

   char fname[];    name of file to read
   boolean lockfl;      check for file locks?
 */
int readin( const char *fname, boolean lockfl) {
    fio_code s ;

    buffer_p bp = curbp ;   /* Cheap.               */
#if (FILOCK && BSD) || SVR4
    if( lockfl && lockchk( fname) == ABORT) {
# if PKCODE
        s = FIOFNF;
        strcpy(bp->b_fname, "");
        mloutstr( "(File in use)") ;
# else
        return ABORT;
# endif
    } else
#endif
  {
    int status = bclear( bp) ;
    if( status != TRUE)   /* Might be old.        */
        return status ;

    bp->b_flag &= ~(BFINVS | BFCHG);
    if( fname != bp->b_fname)   /* Copy if source differs from destination */
        mystrscpy( bp->b_fname, fname, sizeof( fname_t)) ;

    /* let a user macro get hold of things...if he wants */
    execute(META | SPEC | 'R', FALSE, 1);

    s = ffropen( bp->b_fname) ; /* Always use the name associated to buffer */
    if( s == FIOFNF)    /* File not found.      */
        mloutstr( "(New file)") ;
    else if( s == FIOSUC) {
        char *errmsg ;
        eoltype found_eol ;
        int nline = 0 ;

    /* read the file in */
        mloutstr( "(Reading file)") ;
        while( (s = ffgetline()) == FIOSUC) {
            line_p  lp ;

            if( nline >= 10000000 /* Maximum # of lines from one file */
            ||  (lp = lalloc( fpayload)) == NULL) {
                s = FIOMEM ;    /* Keep message on the  */
                break ;         /* display.             */
            }

            memcpy( lp->l_text, fline, fpayload) ;
            lp->l_fp = curbp->b_linep ; /* insert before end of buffer */
            lp->l_bp = lp->l_fp->l_bp ;
            lp->l_fp->l_bp = lp ;
            lp->l_bp->l_fp = lp ;
            nline += 1 ;
        }

        if( s == FIOERR)
            mloutstr( "File read error") ;

        switch( ftype) {
        case FTYPE_DOS:
            found_eol = EOL_DOS ;
            curbp->b_mode |= MDDOS ;
            break ;
        case FTYPE_UNIX:
            found_eol = EOL_UNIX ;
            break ;
        case FTYPE_MAC:
            found_eol = EOL_MAC ;
            curbp->b_mode |= MDMAC ;
            break ;
        case FTYPE_NONE:
            found_eol = EOL_NONE ;
            break ;
        default:
            found_eol = EOL_MIXED ;
            curbp->b_mode |= MDVIEW ;   /* force view mode as we have lost
                                        ** EOL information */
        }

        if( fcode == FCODE_UTF_8)
            curbp->b_mode |= MDUTF8 ;

        if( s == FIOERR
        ||  s == FIOMEM) {
            errmsg = (s == FIOERR) ? "I/O ERROR, " : "OUT OF MEMORY, " ;
            curbp->b_flag |= BFTRUNC ;
            curbp->b_mode |= MDVIEW ;   /* force view mode as lost data */
        } else
            errmsg = "" ;

        mloutfmt( "(%sRead %d line%s, code/eol: %s/%s)",
            errmsg,
            nline,
            &"s"[ nline == 1],
            codename[ fcode],
            eolname[ found_eol]) ;
        ffclose() ; /* Ignore errors.       */
    }
  }

    for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
        if( wp->w_bufp == curbp) {
            wp->w_linep = lforw( curbp->b_linep) ;
            wp->w_dotp = lforw(curbp->b_linep) ;
            wp->w_doto = 0 ;
            wp->w_markp = NULL ;
            wp->w_marko = 0 ;
            wp->w_flag |= WFMODE | WFHARD ;
        }
    }

    return (s == FIOERR || s == FIOFNF) ? FALSE : TRUE ;
}


/* Take a file name, and from it fabricate a buffer name.  This routine
   knows about the syntax of file names on the target system.  I suppose
   that this information could be put in a better place than a line of
   code.
 */
void makename( bname_t bname, const char *fname) {
    const char *cp1 = fname ;
    for( const char *cp = cp1 ; *cp ; cp++)
        if( *cp == '/')
            cp1 = cp + 1 ;

    char *cp2 = bname ;
    while( *cp1 != 0 && *cp1 != ';') {
        unicode_t c ;

        int n = utf8_to_unicode( cp1, 0, 4, &c) ;
        if( cp2 + n <= &bname[ sizeof( bname_t) - 2])   /* 1 digit buffer name conflict [0..9] + EOS */
            while( n--)
                *cp2++ = *cp1++ ;
        else
            break ;
    }

    *cp2 = 0 ;
}


/* make sure a buffer name is unique
 *
 * char *name;      name to check on
 */
void unqname( char *name) {
/* check to see if it is in the buffer list */
    while( bfind( name, 0, FALSE) != NULL) {
    /* go to the end of the name */
        char *sp = name ;
        while( *sp)
            ++sp ;

        if( sp == name || (*(sp - 1) < '0' || *(sp - 1) > '8')) {
            *sp++ = '0' ;
            *sp = 0 ;
        } else
            *(--sp) += 1 ;
    }
}


/* Ask for a file name, and write the content of the current buffer to that
 * file.  Update the remembered file name and clear the buffer changed
 * flag.  Bound to C-X C-W write-file.
 */
BINDABLE( filewrite) {
    char *fname ;

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    int status = newmlarg( &fname, "write-file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
        if( strlen( fname) > sizeof( fname_t) - 1)
            status = FALSE ;
        else {
            status = writeout( fname) ;
            if( status == TRUE)
                strcpy( curbp->b_fname, fname) ;
        }

        free( fname) ;
    }

    return status ;
}


/* Save the content of the current buffer in its associated file.  Do
   nothing if nothing has changed (this may be a bug, not a feature).
   Error if there is no remembered file name for the buffer.  Bound to "C-X
   C-S save-file".  May get called by "M-Z quick-exit".
 */
BINDABLE( filesave) {
    assert( !(curbp->b_mode & MDVIEW)) ;

    if( (curbp->b_flag & BFCHG) == 0)   /* Return, no changes. */
        return TRUE ;

    if( curbp->b_fname[0] == 0) {       /* Must have a name. */
        mloutfmt( "%BNo file name") ;
        return FALSE ;
    }

    /* complain about truncated files */
    if( (curbp->b_flag & BFTRUNC) != 0
    &&  mlyesno( "Truncated file ... write it out") == FALSE)
        return mloutfail( "(Aborted)") ;

    return writeout( curbp->b_fname) ;
}


/* This function performs the details of file writing.  Uses the file
   management routines in the "fileio.c" package.  The number of lines
   written is displayed.  Sadly, it looks inside a struct line; provide a
   macro for this.  Most of the grief is error checking of some sort.
 */
int writeout( const char *fn) {
    fio_code    s ;

    s = ffwopen( fn) ;  /* Open writes message. */
    if( s != FIOSUC)
        mloutstr( "Cannot open file for writing") ;
    else {
        line_p lp ;
        fio_code s2 ;
        int nline = 0 ;

        mloutstr( "(Writing...)") ; /* tell us we are writing */
        for( lp = lforw( curbp->b_linep) ; lp != curbp->b_linep ; lp = lforw( lp)) {
            s = ffputline( lp->l_text, llength( lp), curbp->b_mode >> 8) ; // Mack Hack
            if( s != FIOSUC)
                break ;

            nline += 1 ;
        }

        s2 = ffclose() ;
        if( s != FIOSUC)
            mloutstr( "Write I/O error") ;
        else if( s2 != FIOSUC)
            mloutstr( "Error closing file") ;
        else {  /* Successfull write and close. */
            mloutfmt( "(Wrote %d line%s)", nline, &"s"[ nline == 1]) ;
            curbp->b_flag &= ~BFCHG ;
            upd_mode() ;
            return TRUE ;
        }
    }

    return FALSE ;
}


/* The command allows the user to modify the file name associated with the
   current buffer.  It is like the "f" command in UNIX "ed".  The operation
   is simple; just zap the name in the buffer structure, and mark the
   windows as needing an update.  You can type a blank line at the prompt
   if you wish.  Bound to C-X N change-file-name.
 */
BINDABLE( filename) {
    char *fname ;

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    int status = newmlarg( &fname, "Name: ", sizeof( fname_t)) ;
    if( status == ABORT)
        return status ;
    else if( status == FALSE)
        curbp->b_fname[ 0] = '\0' ;
    else { /* TRUE */
        mystrscpy( curbp->b_fname, fname, sizeof( fname_t)) ;
        free( fname) ;
    }

    curbp->b_mode &= ~MDVIEW ;   /* no longer read only mode */
    upd_mode() ;
    return TRUE ;
}


/* Insert file "fname" into the current buffer, Called by insert file
   command.  Return the final status of the read.
 */
static int ifile( const char *fname) {
    fio_code    s ;

    curbp->b_flag |= BFCHG ;    /* we have changed      */
    curbp->b_flag &= ~BFINVS ;  /* and are not temporary */
    s = ffropen( fname) ;
    if( s == FIOFNF) {  /* File not found.      */
        mloutstr( "(No such file)") ;
        return FALSE ;
    }

    if( s == FIOSUC) {  /* Hard file open.      */
        int nline = 0 ; /* number of line read */
        char *errmsg ;

        mloutstr( "(Inserting file)") ;

    /* back up a line and save the mark here */
        curwp->w_dotp = lback(curwp->w_dotp);
        curwp->w_doto = 0;
        curwp->w_markp = curwp->w_dotp;
        curwp->w_marko = 0;

        while( (s = ffgetline()) == FIOSUC) {
            line_p lpp, lp, lpn ;

            if( (lp = lalloc( fpayload)) == NULL) {
                s = FIOMEM ;    /* Keep message on the  */
                break ;         /* display.             */
            }

            memcpy( lp->l_text, fline, fpayload) ;
            lp->l_bp = lpp = curwp->w_dotp ;    /* insert after dot line */
            lp->l_fp = lpn = lpp->l_fp ;        /* line after insert */

        /* re-link new line between lpp and lpn */
            lpn->l_bp = lp ;
            lpp->l_fp = lp ;

        /* and advance and write out the current line */
            curwp->w_dotp = lp ;
            nline += 1 ;
        }

        ffclose() ; /* Ignore errors.       */
        curwp->w_markp = lforw(curwp->w_markp);
        if( s == FIOERR) {
            errmsg = "I/O ERROR, " ;
            curbp->b_flag |= BFTRUNC ;
        } else if( s == FIOMEM) {
            errmsg = "OUT OF MEMORY, " ;
            curbp->b_flag |= BFTRUNC;
        } else
            errmsg = "" ;

        mloutfmt( "(%sInserted %d line%s)",
            errmsg,
            nline,
            &"s"[ nline == 1]) ;
    }

/* advance to the next line and mark the window for changes */
    curwp->w_dotp = lforw(curwp->w_dotp);
    curwp->w_flag |= WFHARD | WFMODE;

/* copy window parameters back to the buffer structure */
    curbp->b_dotp = curwp->w_dotp;
    curbp->b_doto = curwp->w_doto;
    curbp->b_markp = curwp->w_markp;
    curbp->b_marko = curwp->w_marko;

    return (s == FIOERR) ? FALSE : TRUE ;
}

/* end of file.c */
