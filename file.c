/* file.c -- implements file.h */

#include "file.h"

/*  file.c
 *
 *  The routines in this file handle the reading, writing
 *  and lookup of disk files.  All of details about the
 *  reading and writing of the disk are in "fileio.c".
 *
 *  modified by Petri Kutvonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "defines.h"
#include "display.h"
#include "estruct.h"
#include "execute.h"
#include "fileio.h"
#include "input.h"
#include "line.h"
#include "lock.h"
#include "mlout.h"
#include "window.h"

#if PKCODE
/* Max number of lines from one file. */
#define MAXNLINE 10000000
#endif

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

boolean restflag = FALSE ;	/* restricted use?              */


static int ifile( const char *fname) ;


boolean resterr( void) {
	mloutfmt( "%B(That command is RESTRICTED)") ;
	return FALSE ;
}

/*
 * Read a file into the current
 * buffer. This is really easy; all you do it
 * find the name of the file, and call the standard
 * "read a file into the current buffer" code.
 * Bound to "C-X C-R".
 */
int fileread( int f, int n) {
    int status ;
    char *fname ;

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    status = newmlarg( &fname, "Read file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
    	status = readin( fname, TRUE) ;
    	free( fname) ;
    }

	return status ;
}

/*
 * Insert a file into the current
 * buffer. This is really easy; all you do it
 * find the name of the file, and call the standard
 * "insert a file into the current buffer" code.
 * Bound to "C-X C-I".
 */
int insfile( int f, int n) {
    int status ;
    char *fname ;

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    if( curbp->b_mode & MDVIEW)	/* don't allow this command if	*/
        return rdonly() ;		/* we are in read only mode     */

    status = newmlarg( &fname, "Insert file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
	    status = ifile( fname) ;
	    free( fname) ;
	}

	if( status != TRUE)
        return status ;

    return reposition( TRUE, -1) ;
}

/*
 * Select a file for editing.
 * Look around to see if you can find the
 * fine in another buffer; if you can find it
 * just switch to the buffer. If you cannot find
 * the file, create a new buffer, read in the
 * text, and switch to the new buffer.
 * Bound to C-X C-F.
 */
int filefind( int f, int n) {
    char *fname ;	/* file user wishes to find */
    int status ;	/* status return */

    if( restflag)	/* don't allow this command if restricted */
        return resterr() ;

    status = newmlarg( &fname, "Find file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
		status = getfile( fname, TRUE) ;
		free( fname) ;
	}

	return status ;
}

int viewfile( int f, int n) {	/* visit a file in VIEW mode */
    char *fname ;	/* file user wishes to find */
    int	status ;	/* status return */

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;
        
    status = newmlarg( &fname, "View file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
		status = getfile(fname, FALSE) ;
		free( fname) ;
	}

    if( status == TRUE) {	/* if we succeed, put it in view mode */
	    struct window *wp ;	/* scan for windows that need updating */

        curwp->w_bufp->b_mode |= MDVIEW ;

        /* scan through and update mode lines of all windows */
        wp = wheadp ;
        while( wp != NULL) {
            wp->w_flag |= WFMODE ;
            wp = wp->w_wndp ;
        }
    }

    return status ;
}

/*
 * getfile()
 *
 * char fname[];    file name to find
 * boolean lockfl;      check the file for locks?
 */
int getfile( const char *fname, boolean lockfl)
{
    struct buffer *bp;
    struct line *lp;
    int i;
    int s;
    bname_t bname ;  /* buffer name to put file */

#if MSDOS
    mklower(fname);     /* msdos isn't case sensitive */
#endif
    for (bp = bheadp; bp != NULL; bp = bp->b_bufp) {
        if ((bp->b_flag & BFINVS) == 0
            && strcmp(bp->b_fname, fname) == 0) {
            swbuffer(bp);
            lp = curwp->w_dotp;
            i = curwp->w_ntrows / 2;
            while (i-- && lback(lp) != curbp->b_linep)
                lp = lback(lp);
            curwp->w_linep = lp;
            curwp->w_flag |= WFMODE | WFHARD;
            cknewwindow();
            mloutstr( "(Old buffer)") ;
            return TRUE;
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
        	strncpy( bname, new_bname, sizeof bname - 1) ;
        	bname[ sizeof bname - 1] = '\0' ;
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

/*
 * Read file "fname" into the current buffer, blowing away any text
 * found there.  Called by both the read and find commands.  Return
 * the final status of the read.  Also called by the mainline, to
 * read in a file specified on the command line as an argument.
 * The command bound to M-FNR is called after the buffer is set up
 * and before it is read.
 *
 * char fname[];    name of file to read
 * boolean lockfl;      check for file locks?
 */
int readin(const char *fname, boolean lockfl)
{
    struct line *lp1;
    struct line *lp2;
    struct window *wp;
    struct buffer *bp;
    int s;
    eoltype found_eol ;
    int nbytes;
    int nline;
	char *errmsg ;

#if (FILOCK && BSD) || SVR4
    if (lockfl && lockchk(fname) == ABORT)
#if PKCODE
    {
        s = FIOFNF;
        bp = curbp;
        strcpy(bp->b_fname, "");
        goto out;
    }
#else
        return ABORT;
#endif
#endif
    bp = curbp;     /* Cheap.               */
    if ((s = bclear(bp)) != TRUE)   /* Might be old.        */
        return s;
    bp->b_flag &= ~(BFINVS | BFCHG);
    strncpy( bp->b_fname, fname, sizeof( fname_t) - 1) ;
    bp->b_fname[ sizeof( fname_t) - 1] = '\0' ;

    /* let a user macro get hold of things...if he wants */
    execute(META | SPEC | 'R', FALSE, 1);

    if ((s = ffropen(fname)) == FIOERR) /* Hard file open.      */
        goto out;

    if (s == FIOFNF) {  /* File not found.      */
        mloutstr( "(New file)") ;
        goto out;
    }

    /* read the file in */
    mloutstr( "(Reading file)") ;
    nline = 0;
    while ((s = ffgetline()) == FIOSUC) {
        nbytes = fpayload ;
#if PKCODE
        if (nline > MAXNLINE) {
            s = FIOMEM;
            break;
        }
#endif
        if ((lp1 = lalloc(nbytes)) == NULL) {
            s = FIOMEM; /* Keep message on the  */
            break;  /* display.             */
        }
        lp2 = lback(curbp->b_linep);
        lp2->l_fp = lp1;
        lp1->l_fp = curbp->b_linep;
        lp1->l_bp = lp2;
        curbp->b_linep->l_bp = lp1;
		memcpy( lp1->l_text, fline, nbytes) ;
        ++nline;
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
    	break ;
    case FTYPE_NONE:
    	found_eol = EOL_NONE ;
    	break ;
    default:
    	found_eol = EOL_MIXED ;
		curbp->b_mode |= MDVIEW ;	/* force view mode as we have lost
									** EOL information */
	}

	if( fcode == FCODE_UTF_8)
		curbp->b_mode |= MDUTF8 ;

    if( s == FIOERR) {
		errmsg = "I/O ERROR, " ;
        curbp->b_flag |= BFTRUNC ;
    } else if( s == FIOMEM) {
        errmsg = "OUT OF MEMORY, " ;
        curbp->b_flag |= BFTRUNC ;
    } else
    	errmsg = "" ;

	mloutfmt( "(%sRead %d line%s, code/eol: %s/%s)",
		errmsg,
		nline,
		(nline != 1) ? "s" : "",
		codename[ fcode & (FCODE_MASK -1)],
		eolname[ found_eol]) ;
    ffclose();      /* Ignore errors.       */

      out:
    for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
        if (wp->w_bufp == curbp) {
            wp->w_linep = lforw(curbp->b_linep);
            wp->w_dotp = lforw(curbp->b_linep);
            wp->w_doto = 0;
            wp->w_markp = NULL;
            wp->w_marko = 0;
            wp->w_flag |= WFMODE | WFHARD;
        }
    }
    if (s == FIOERR || s == FIOFNF) /* False if error.      */
        return FALSE;
    return TRUE;
}

/*
 * Take a file name, and from it
 * fabricate a buffer name. This routine knows
 * about the syntax of file names on the target system.
 * I suppose that this information could be put in
 * a better place than a line of code.
 */
void makename( bname_t bname, const char *fname)
{
    const char *cp1;
    char *cp2;

    cp1 = &fname[0];
    while (*cp1 != 0)
        ++cp1;

#if     VMS
#if PKCODE
    while (cp1 != &fname[0] && cp1[-1] != ':' && cp1[-1] != ']'
           && cp1[-1] != '>')
#else
    while (cp1 != &fname[0] && cp1[-1] != ':' && cp1[-1] != ']')
#endif
        --cp1;
#endif
#if     MSDOS
    while (cp1 != &fname[0] && cp1[-1] != ':' && cp1[-1] != '\\'
           && cp1[-1] != '/')
        --cp1;
#endif
#if     V7 | USG | BSD
    while (cp1 != &fname[0] && cp1[-1] != '/')
        --cp1;
#endif
    cp2 = &bname[0];
    while( cp2 != &bname[ sizeof( bname_t) - 1] && *cp1 != 0 && *cp1 != ';')
        *cp2++ = *cp1++;
    *cp2 = 0;
}

/*
 * make sure a buffer name is unique
 *
 * char *name;      name to check on
 */
void unqname(char *name)
{
    char *sp;

    /* check to see if it is in the buffer list */
    while (bfind(name, 0, FALSE) != NULL) {

        /* go to the end of the name */
        sp = name;
        while (*sp)
            ++sp;
        if (sp == name || (*(sp - 1) < '0' || *(sp - 1) > '8')) {
            *sp++ = '0';
            *sp = 0;
        } else
            *(--sp) += 1;
    }
}

/*
 * Ask for a file name, and write the
 * contents of the current buffer to that file.
 * Update the remembered file name and clear the
 * buffer changed flag. This handling of file names
 * is different from the earlier versions, and
 * is more compatable with Gosling EMACS than
 * with ITS EMACS. Bound to "C-X C-W".
 */
int filewrite( int f, int n) {
    int status ;
    char *fname ;

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    status = newmlarg( &fname, "Write file: ", sizeof( fname_t)) ;
    if( status == TRUE) {
		if( strlen( fname) > sizeof( fname_t) - 1)
			status = FALSE ;
		else {
		    status = writeout( fname) ;
		    if( status == TRUE) {
			    struct window *wp ;

	    	    strcpy( curbp->b_fname, fname) ;
	        	curbp->b_flag &= ~BFCHG ;
		        wp = wheadp ;	/* Update mode lines.   */
		        while( wp != NULL) {
	    	        if( wp->w_bufp == curbp)
	        	        wp->w_flag |= WFMODE ;

	        	    wp = wp->w_wndp ;
				}
			}
		}
    
		free( fname) ;
	}

    return status ;
}

/*
 * Save the contents of the current
 * buffer in its associatd file. No nothing
 * if nothing has changed (this may be a bug, not a
 * feature). Error if there is no remembered file
 * name for the buffer. Bound to "C-X C-S". May
 * get called by "C-Z".
 */
int filesave(int f, int n)
{
    struct window *wp;
    int s;

    if (curbp->b_mode & MDVIEW) /* don't allow this command if      */
        return rdonly();    /* we are in read only mode     */
    if ((curbp->b_flag & BFCHG) == 0)   /* Return, no changes.  */
        return TRUE;
    if (curbp->b_fname[0] == 0) {   /* Must have a name.    */
        mloutstr( "No file name") ;
        return FALSE;
    }

    /* complain about truncated files */
    if ((curbp->b_flag & BFTRUNC) != 0) {
        if (mlyesno("Truncated file ... write it out") == FALSE) {
            mloutstr( "(Aborted)") ;
            return FALSE;
        }
    }

    if ((s = writeout(curbp->b_fname)) == TRUE) {
        curbp->b_flag &= ~BFCHG;
        wp = wheadp;    /* Update mode lines.   */
        while (wp != NULL) {
            if (wp->w_bufp == curbp)
                wp->w_flag |= WFMODE;
            wp = wp->w_wndp;
        }
    }
    return s;
}

/*
 * This function performs the details of file
 * writing. Uses the file management routines in the
 * "fileio.c" package. The number of lines written is
 * displayed. Sadly, it looks inside a struct line; provide
 * a macro for this. Most of the grief is error
 * checking of some sort.
 */
int writeout( const char *fn)
{
    int s;
    struct line *lp;
    int nline;

    if ((s = ffwopen(fn)) != FIOSUC) {  /* Open writes message. */
        mloutstr( "Cannot open file for writing") ;
        return FALSE;
    }
    mloutstr( "(Writing...)") ;	/* tell us were writing */
    lp = lforw(curbp->b_linep); /* First line.          */
    nline = 0;      /* Number of lines.     */
    while (lp != curbp->b_linep) {
        s = ffputline( &lp->l_text[0], llength(lp), curbp->b_mode & MDDOS) ;
        if( s != FIOSUC) {
	        mloutstr( "Write I/O error") ;
            break;
        }
        
        ++nline;
        lp = lforw(lp);
    }
    if (s == FIOSUC) {  /* No write error.      */
        s = ffclose();
        if (s == FIOSUC) {  /* No close error.      */
            if (nline == 1)
                mloutstr( "(Wrote 1 line)") ;
            else
                mloutfmt( "(Wrote %d lines)", nline) ;
        } else
			mloutstr( "Error closing file") ;
    } else          /* Ignore close error   */
        ffclose();  /* if a write error.    */
    if (s != FIOSUC)    /* Some sort of error.  */
        return FALSE;
    return TRUE;
}

/*
 * The command allows the user
 * to modify the file name associated with
 * the current buffer. It is like the "f" command
 * in UNIX "ed". The operation is simple; just zap
 * the name in the buffer structure, and mark the windows
 * as needing an update. You can type a blank line at the
 * prompt if you wish.
 */
int filename( int f, int n) {
    struct window *wp ;
    int status ;
    char *fname ;

    if( restflag)       /* don't allow this command if restricted */
        return resterr() ;

    status = newmlarg( &fname, "Name: ", sizeof( fname_t)) ;
    if( status == ABORT)
        return status ;
    else if( status == FALSE)
        curbp->b_fname[ 0] = '\0' ;
    else { /* TRUE */
        strncpy( curbp->b_fname, fname, sizeof( fname_t) - 1) ;
        curbp->b_fname[ sizeof( fname_t) - 1] = '\0' ;
        free( fname) ;
    }

    wp = wheadp ;        /* Update mode lines.   */
    while( wp != NULL) {
        if( wp->w_bufp == curbp)
            wp->w_flag |= WFMODE ;

        wp = wp->w_wndp ;
    }

    curbp->b_mode &= ~MDVIEW ;   /* no longer read only mode */
    return TRUE ;
}

/*
 * Insert file "fname" into the current
 * buffer, Called by insert file command. Return the final
 * status of the read.
 */
static int ifile( const char *fname) {
    struct line *lp0;
    struct line *lp1;
    struct line *lp2;
    struct buffer *bp;
    int s;
    int nbytes;
    int nline;
    char *errmsg ;

    bp = curbp;     /* Cheap.               */
    bp->b_flag |= BFCHG;    /* we have changed      */
    bp->b_flag &= ~BFINVS;  /* and are not temporary */
    if ((s = ffropen(fname)) == FIOERR) /* Hard file open.      */
        goto out;
    if (s == FIOFNF) {  /* File not found.      */
        mloutstr( "(No such file)") ;
        return FALSE;
    }
    mloutstr( "(Inserting file)") ;

    /* back up a line and save the mark here */
    curwp->w_dotp = lback(curwp->w_dotp);
    curwp->w_doto = 0;
    curwp->w_markp = curwp->w_dotp;
    curwp->w_marko = 0;

    nline = 0;
    while ((s = ffgetline()) == FIOSUC) {
        nbytes = fpayload ;
        if ((lp1 = lalloc(nbytes)) == NULL) {
            s = FIOMEM; /* Keep message on the  */
            break;  /* display.             */
        }
        lp0 = curwp->w_dotp;    /* line previous to insert */
        lp2 = lp0->l_fp;    /* line after insert */

        /* re-link new line between lp0 and lp2 */
        lp2->l_bp = lp1;
        lp0->l_fp = lp1;
        lp1->l_bp = lp0;
        lp1->l_fp = lp2;

        /* and advance and write out the current line */
        curwp->w_dotp = lp1;
		memcpy( lp1->l_text, fline, nbytes) ;
        ++nline;
    }
    ffclose();      /* Ignore errors.       */
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
		(nline > 1) ? "s" : "") ;

out:
    /* advance to the next line and mark the window for changes */
    curwp->w_dotp = lforw(curwp->w_dotp);
    curwp->w_flag |= WFHARD | WFMODE;

    /* copy window parameters back to the buffer structure */
    curbp->b_dotp = curwp->w_dotp;
    curbp->b_doto = curwp->w_doto;
    curbp->b_markp = curwp->w_markp;
    curbp->b_marko = curwp->w_marko;

    if (s == FIOERR)    /* False if error.      */
        return FALSE;
    return TRUE;
}
