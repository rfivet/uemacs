/* basic.c -- implements basic.h */
#include "basic.h"

/* The routines in this file move the cursor around on the screen.  They
   compute a new value for the cursor, then adjust ".".  The display code
   always updates the cursor location, so only moves between lines, or
   functions that adjust the top line in the window and invalidate the
   framing, are hard.

   modified by Petri Kutvonen
 */

#include <assert.h>
#include <stdlib.h>

#include "input.h"
#include "mlout.h"
#include "random.h"
#include "terminal.h"
#include "window.h"

#define CVMVAS  1   /* arguments to page forward/back in pages */

int overlap = 0 ;   /* $overlap: line overlap in forw/back page */
int curgoal ;       /* $target:  column goal for C-P, C-N */


/* This routine, given a pointer to a struct line, and the current cursor
   goal column, return the best choice for the offset.  The offset is
   returned.  Used by "C-N" and "C-P".
 */
static unsigned getgoal( line_p dlp) {
    int col = 0 ;
    const unsigned len = llength( dlp) ;
    unsigned idx = 0 ;
    while( idx < len) {
        unicode_t c ;
        unsigned width = utf8_to_unicode( dlp->l_text, idx, len, &c) ;

    /* Take tabs, ^X and \xx hex characters into account */
        if( c == '\t')
            col += tabwidth - col % tabwidth ;
        else if( c < 0x20 || c == 0x7F)     /* ^x */
            col += 2 ;
        else if( c >= 0x80 && c <= 0xA0)    /* \xx */
            col += 3 ;
        else
            col += utf8_width( c) ;

        if( col > curgoal)
            break ;
        else
            idx += width ;
    }

    return idx ;
}


/* Move the cursor to the beginning of the current line of active window. */
TBINDABLE( gotobol) {
    curwp->w_doto = 0 ;
    return TRUE ;
}


/* Move the cursor to the end of the current line of active window. */
TBINDABLE( gotoeol) {
    curwp->w_doto = llength( curwp->w_dotp) ;
    return TRUE ;
}


/* Goto the beginning of the buffer.  Massive adjustment of dot.  This is
   considered to be hard motion; it really isn't if the original value of
   dot is the same as the new value of dot.  Normally bound to "M-<".
 */
TBINDABLE( gotobob) {
    curwp->w_dotp = lforw( curbp->b_linep) ;
    curwp->w_doto = 0 ;
    curwp->w_flag |= WFHARD ;
    return TRUE ;
}


/* Move to the end of the buffer.  Dot is always put at the end of the file
   (ZJ).  The standard screen code does most of the hard parts of update.
   Bound to "M->".
 */
TBINDABLE( gotoeob) {
    curwp->w_dotp = curbp->b_linep ;
    curwp->w_doto = 0 ;
    curwp->w_flag |= WFHARD ;
    return TRUE ;
}


/* Move forward by full lines.  If the number of lines to move is less than
   zero, call the backward line function to actually do it.  The last
   command controls how the goal column is set.  Bound to "C-N".  No errors
   are possible.
 */
BBINDABLE( forwline) {
    assert( f == TRUE || n == 1) ;

/* if the last command was not a line move, reset the goal column */
    if( (lastflag & CFCPCN) == 0)
        curgoal = getccol( FALSE) ;

/* flag this command as a line move */
    thisflag |= CFCPCN ;

/* and move the point down */
    if( n) {
        line_p dlp = curwp->w_dotp ;
        if( n > 0)
            while( n && dlp != curbp->b_linep) {
                dlp = lforw( dlp) ;
                n -= 1 ;
            }
        else {
            while( n && lback( dlp) != curbp->b_linep) {
                dlp = lback( dlp) ;
                n += 1 ;
            }
        }

    /* resetting the current position */
        curwp->w_dotp = dlp ;
        curwp->w_doto = getgoal( dlp) ;
        curwp->w_flag |= WFMOVE ;
    }

    return (n == 0) ? TRUE : FALSE ;
}


/* This function is like "forwline", but goes backwards.  The scheme is
   exactly the same.  Check for arguments that are less than zero and call
   your alternate.  Figure out the new line and call "movedot" to perform
   the motion.  No errors are possible.  Bound to "C-P".
 */
BBINDABLE( backline) {
    assert( f == TRUE || n == 1) ;

    return forwline( TRUE, -n) ;
}


/* Move to a particular line.
 *
 * @n: The specified line position at the current buffer.
 */
BINDABLE( gotoline) {
/* Get an argument if one doesn't exist. */
    if( f == FALSE) {
        char *arg ; /* Buffer to hold argument. */

        int status = newmlarg( &arg, "goto-line: ", 0) ;
        if( status != TRUE)
            return status ;

        n = atoi( arg) ;
        free( arg) ;
        f = TRUE ;
    }

/* Handle the case where the user may be passed something like this:
 *  ue filename +
 * In this case we just go to the end of the buffer.
 */
    if( n == 0)
        return gotoeob( f, n) ;

/* If a bogus argument was passed, then returns false. */
    if( n < 0)
        return FALSE ;

/* First, we go to the begin of the buffer. */
    gotobob( f, n) ;
    return (n == 1) ? TRUE : forwline( TRUE, n - 1) ;
}


/* Scroll forward by a specified number of lines, or by a full page if no
   argument.  Bound to "C-V".  The "2" in the arithmetic on the window size
   is the overlap; this value is the default overlap value in ITS EMACS.
   Because this zaps the top line in the display window, we have to do a
   hard update.
 */
TBINDABLE( forwpage) {
    line_p lp ;

    if( f == FALSE) {
#if SCROLLCODE
        if (term.t_scroll != NULL)  /* $scroll == FALSE */
            if (overlap == 0)       /* $overlap == 0 */
                n = curwp->w_ntrows * 2 / 3 ;
            else
                n = curwp->w_ntrows - overlap;
        else
#endif
            n = curwp->w_ntrows - 2;  /* Default scroll. */

        if (n <= 0) /* Forget the overlap. */
            n = 1;  /* If tiny window. */
    } else if( n < 0)
        return backpage( f, -n) ;
#if     CVMVAS
    else            /* Convert from pages. */
        n *= curwp->w_ntrows;   /* To lines. */
#endif

    lp = curwp->w_dotp ;
    while( n && lp != curbp->b_linep) {
        lp = lforw( lp) ;
        n -= 1 ;
    }

    curwp->w_dotp = lp ;
    curwp->w_doto = 0 ;
    reposition( TRUE, 0) ;  /* center at dot, always succeed */

#if SCROLLCODE
    curwp->w_flag |= WFHARD | WFKILLS;
#else
    curwp->w_flag |= WFHARD;
#endif
    return TRUE ;
}


/* This command is like "forwpage", but it goes backwards.  The "2", like
   above, is the overlap between the two windows.  The value is from the
   ITS EMACS manual.  Bound to "M-V".  We do a hard update for exactly the
   same reason.
 */
TBINDABLE( backpage) {
    line_p lp ;

    if( f == FALSE) {   /* interactive, default n = 1 supplied */
    /* in interactive mode, first move dot to top of window */
        if( curwp->w_dotp != curwp->w_linep) {
            curwp->w_dotp = curwp->w_linep ;
            curwp->w_doto = 0 ;
/*          curwp->w_flag |= WFMOVE ; */
            return TRUE ;
        }

#if SCROLLCODE
        if (term.t_scroll != NULL)  /* $scroll != FALSE */
            if (overlap == 0)       /* $overlap == 0 */
                n = curwp->w_ntrows * 2 / 3 ;
            else
                n = curwp->w_ntrows - overlap;
        else
#endif
            n = curwp->w_ntrows - 2; /* Default scroll. */

        if (n <= 0) /* Don't blow up if the. */
            n = 1;  /* Window is tiny. */
    } else if (n < 0)
        return forwpage(f, -n);
#if     CVMVAS
    else  /* Convert from pages. */
        n *= curwp->w_ntrows;  /* To lines. */
#endif

/*  lp = curwp->w_linep; */
    lp = curwp->w_dotp ;
    while( n && lback( lp) != curbp->b_linep) {
        lp = lback( lp) ;
        n -= 1 ;
    }

/*  curwp->w_linep = lp; */
    curwp->w_dotp = lp;
    curwp->w_doto = 0;
    reposition( TRUE, (f == FALSE) ? 1 : 0) ;

#if SCROLLCODE
    curwp->w_flag |= WFHARD | WFINS;
#else
    curwp->w_flag |= WFHARD;
#endif
    return TRUE;
}


/* Set the mark in the current window to the value of "." in the window.
   No errors are possible.  Bound to M-. set-mark.
 */
TBINDABLE( setmark) {
    curwp->w_markp = curwp->w_dotp ;
    curwp->w_marko = curwp->w_doto ;
    mloutstr( "(Mark set)") ;
    return TRUE ;
}


/* Swap the values of "." and "mark" in the current window.  If no mark as
   been previously set, set it.  Bound to C-X C-X exchange-point-and-mark.
 */
TBINDABLE( swapmark) {
    line_p odotp = curwp->w_dotp ;
    int odoto = curwp->w_doto ;
    if( curwp->w_markp) {
        curwp->w_dotp = curwp->w_markp ;
        curwp->w_doto = curwp->w_marko ;
        curwp->w_flag |= WFMOVE ;
    }

    curwp->w_markp = odotp ;
    curwp->w_marko = odoto ;
    return TRUE ;
}

/* end of basic.c */
