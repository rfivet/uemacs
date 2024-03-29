/* window.c -- inplements window.h */
#include "window.h"

/* Window management.  Some of the functions are internal, and some are
   attached to keys that the user actually types.
 */

#include <assert.h>
#include <stdlib.h>		/* malloc(), free() */

#include "basic.h"
#include "buffer.h"
#include "defines.h"
#include "display.h"	/* upmode() */
#include "execute.h"
#include "line.h"
#include "mlout.h"
#include "terminal.h"
#include "wrapper.h"

window_p curwp ;		/* Current window               */
window_p wheadp ;		/* Head of list of windows      */

static window_p savwindow = NULL ;	/* saved window pointer */


/* Reposition dot in the current window to line "n".  If the argument is
   positive, it is that line.  If it is negative it is that line from the
   bottom.  If it is 0 the window is centered (this is what the standard
   redisplay code does).  With no argument it defaults to 0.  Bound to M-!.
 */
TBINDABLE( reposition) {
	curwp->w_force = (f == FALSE) ? 0 : n ;	/* default to 0 to center screen */
	curwp->w_flag |= WFFORCE ;
	return TRUE ;
}


/* Refresh the screen.  With no argument, it just does the refresh.  With
   an argument it recenters "." in the current window.  Bound to "C-L".
 */
TBINDABLE( redraw) {
	if( f == FALSE)
		sgarbf = TRUE ;
	else {
		curwp->w_force = 0 ;	/* Center dot. */
		curwp->w_flag |= WFFORCE ;
	}

	return TRUE ;
}


/* The command make the next window (next => down the screen) the current
   window.  There are no real errors, although the command does nothing if
   there is only 1 window on the screen.  Bound to "C-X O".

   with an argument this command finds the <n>th window from the top
 
   int f, n;		default flag and numeric argument
 */
BINDABLE( nextwind) {
	window_p wp;
	int nwindows;	/* total number of windows */

	if (f) {

		/* first count the # of windows */
		wp = wheadp;
		nwindows = 1;
		while (wp->w_wndp != NULL) {
			nwindows++;
			wp = wp->w_wndp;
		}

		/* if the argument is negative, it is the nth window
		   from the bottom of the screen                        */
		if (n < 0)
			n = nwindows + n + 1;

		/* if an argument, give them that window from the top */
		if (n > 0 && n <= nwindows) {
			wp = wheadp;
			while (--n)
				wp = wp->w_wndp;
		} else
			return mloutfail( "Window number out of range") ;
	} else if ((wp = curwp->w_wndp) == NULL)
		wp = wheadp;
	curwp = wp;
	curbp = wp->w_bufp;
	cknewwindow();
	upmode();
	return TRUE;
}


/* This command makes the previous window (previous => up the screen) the
   current window.  There arn't any errors, although the command does not
   do a lot if there is 1 window.
 */
BINDABLE( prevwind) {
	window_p wp1;
	window_p wp2;

	/* if we have an argument, we mean the nth window from the bottom */
	if (f)
		return nextwind(f, -n);

	wp1 = wheadp;
	wp2 = curwp;

	if (wp1 == wp2)
		wp2 = NULL;

	while (wp1->w_wndp != wp2)
		wp1 = wp1->w_wndp;

	curwp = wp1;
	curbp = wp1->w_bufp;
	cknewwindow();
	upmode();
	return TRUE;
}


/* This command moves the current window down by "arg" lines. Recompute the
 * top line in the window. The move up and move down code is almost completely
 * the same; most of the work has to do with reframing the window, and picking
 * a new dot. We share the code by having "move down" just be an interface to
 * "move up". Magic. Bound to "C-X C-N".
 */
BINDABLE( mvdnwind) {
	return mvupwind( f, -n) ;
}


/* Move the current window up by "arg" lines. Recompute the new top line of
 * the window. Look to see if "." is still on the screen. If it is, you win.
 * If it isn't, then move "." to center it in the new framing of the window
 * (this command does not really move "."; it moves the frame). Bound to
 * "C-X C-P".
 */
BINDABLE( mvupwind) {
	line_p lp;
	int i;

	lp = curwp->w_linep;

	if (n < 0) {
		while (n++ && lp != curbp->b_linep)
			lp = lforw(lp);
	} else {
		while (n-- && lback(lp) != curbp->b_linep)
			lp = lback(lp);
	}

	curwp->w_linep = lp;
	curwp->w_flag |= WFHARD;	/* Mode line is OK. */

	for (i = 0; i < curwp->w_ntrows; ++i) {
		if (lp == curwp->w_dotp)
			return TRUE;
		if (lp == curbp->b_linep)
			break;
		lp = lforw(lp);
	}

	lp = curwp->w_linep;
	i = curwp->w_ntrows / 2;

	while (i-- && lp != curbp->b_linep)
		lp = lforw(lp);

	curwp->w_dotp = lp;
	curwp->w_doto = 0;
	return TRUE;
}


/* This command makes the current window the only window on the screen. Bound
 * to "C-X 1". Try to set the framing so that "." does not have to move on the
 * display. Some care has to be taken to keep the values of dot and mark in
 * the buffer structures right if the distruction of a window makes a buffer
 * become undisplayed.
 */
BINDABLE( onlywind) {
	window_p wp;
	line_p lp;
	int i;

	while (wheadp != curwp) {
		wp = wheadp;
		wheadp = wp->w_wndp;
		if (--wp->w_bufp->b_nwnd == 0) {
			wp->w_bufp->b_dotp = wp->w_dotp;
			wp->w_bufp->b_doto = wp->w_doto;
			wp->w_bufp->b_markp = wp->w_markp;
			wp->w_bufp->b_marko = wp->w_marko;
		}
		free((char *) wp);
	}
	while (curwp->w_wndp != NULL) {
		wp = curwp->w_wndp;
		curwp->w_wndp = wp->w_wndp;
		if (--wp->w_bufp->b_nwnd == 0) {
			wp->w_bufp->b_dotp = wp->w_dotp;
			wp->w_bufp->b_doto = wp->w_doto;
			wp->w_bufp->b_markp = wp->w_markp;
			wp->w_bufp->b_marko = wp->w_marko;
		}
		free((char *) wp);
	}
	lp = curwp->w_linep;
	i = curwp->w_toprow;
	while (i != 0 && lback(lp) != curbp->b_linep) {
		--i;
		lp = lback(lp);
	}
	curwp->w_toprow = 0;
	curwp->w_ntrows = term.t_nrow - 1;
	curwp->w_linep = lp;
	curwp->w_flag |= WFMODE | WFHARD;
	return TRUE;
}


/* Delete the current window, placing its space in the window above,
 * or, if it is the top window, the window below. Bound to C-X 0.
 *
 * int f, n;	arguments are ignored for this command
 */
BINDABLE( delwind) {
	window_p wp;	/* window to recieve deleted space */
	window_p lwp;	/* ptr window before curwp */
	int target;	/* target line to search for */

	/* if there is only one window, don't delete it */
	if( wheadp->w_wndp == NULL)
		return mloutfail( "Can not delete this window") ;

	/* find window before curwp in linked list */
	wp = wheadp;
	lwp = NULL;
	while (wp != NULL) {
		if (wp == curwp)
			break;
		lwp = wp;
		wp = wp->w_wndp;
	}

	/* find recieving window and give up our space */
	wp = wheadp;
	if (curwp->w_toprow == 0) {
		/* find the next window down */
		target = curwp->w_ntrows + 1;
		while (wp != NULL) {
			if (wp->w_toprow == target)
				break;
			wp = wp->w_wndp;
		}
		if (wp == NULL)
			return FALSE;
		wp->w_toprow = 0;
		wp->w_ntrows += target;
	} else {
		/* find the next window up */
		target = curwp->w_toprow - 1;
		while (wp != NULL) {
			if ((wp->w_toprow + wp->w_ntrows) == target)
				break;
			wp = wp->w_wndp;
		}
		if (wp == NULL)
			return FALSE;
		wp->w_ntrows += 1 + curwp->w_ntrows;
	}

	/* get rid of the current window */
	if (--curwp->w_bufp->b_nwnd == 0) {
		curwp->w_bufp->b_dotp = curwp->w_dotp;
		curwp->w_bufp->b_doto = curwp->w_doto;
		curwp->w_bufp->b_markp = curwp->w_markp;
		curwp->w_bufp->b_marko = curwp->w_marko;
	}
	if (lwp == NULL)
		wheadp = curwp->w_wndp;
	else
		lwp->w_wndp = curwp->w_wndp;
	free((char *) curwp);
	curwp = wp;
	wp->w_flag |= WFHARD;
	curbp = wp->w_bufp;
	cknewwindow();
	upmode();
	return TRUE;
}


/* Split the current window.  A window smaller than 3 lines cannot be
 * split.  An argument of 1 forces the cursor into the upper window, an
 * argument of two forces the cursor to the lower window.  The only
 * other error that is possible is a "malloc" failure allocating the
 * structure for the new window.  Bound to "C-X 2". 
 *
 * int f, n;	default flag and numeric argument
 */
BINDABLE( splitwind) {
	window_p wp;
	line_p lp;
	int ntru;
	int ntrl;
	int ntrd;
	window_p wp1;
	window_p wp2;

	if( curwp->w_ntrows < 3) {
		mloutfmt( "Cannot split a %d line window", curwp->w_ntrows) ;
		return FALSE ;
	}

	wp = malloc( sizeof *wp) ;
	if( wp == NULL)
		return mloutfail( "Out of memory") ;

	++curbp->b_nwnd;	/* Displayed twice.     */
	wp->w_bufp = curbp;
	wp->w_dotp = curwp->w_dotp;
	wp->w_doto = curwp->w_doto;
	wp->w_markp = curwp->w_markp;
	wp->w_marko = curwp->w_marko;
	wp->w_flag = 0;
	wp->w_force = 0;
#if	COLOR
	/* set the colors of the new window */
	wp->w_fcolor = gfcolor;
	wp->w_bcolor = gbcolor;
#endif
	ntru = (curwp->w_ntrows - 1) / 2;	/* Upper size           */
	ntrl = (curwp->w_ntrows - 1) - ntru;	/* Lower size           */
	lp = curwp->w_linep;
	ntrd = 0;
	while (lp != curwp->w_dotp) {
		++ntrd;
		lp = lforw(lp);
	}
	lp = curwp->w_linep;
	if (((f == FALSE) && (ntrd <= ntru)) || ((f == TRUE) && (n == 1))) {
		/* Old is upper window. */
		if (ntrd == ntru)	/* Hit mode line.       */
			lp = lforw(lp);
		curwp->w_ntrows = ntru;
		wp->w_wndp = curwp->w_wndp;
		curwp->w_wndp = wp;
		wp->w_toprow = curwp->w_toprow + ntru + 1;
		wp->w_ntrows = ntrl;
	} else {		/* Old is lower window  */
		wp1 = NULL;
		wp2 = wheadp;
		while (wp2 != curwp) {
			wp1 = wp2;
			wp2 = wp2->w_wndp;
		}
		if (wp1 == NULL)
			wheadp = wp;
		else
			wp1->w_wndp = wp;
		wp->w_wndp = curwp;
		wp->w_toprow = curwp->w_toprow;
		wp->w_ntrows = ntru;
		++ntru;		/* Mode line.           */
		curwp->w_toprow += ntru;
		curwp->w_ntrows = ntrl;
		while (ntru--)
			lp = lforw(lp);
	}
	curwp->w_linep = lp;	/* Adjust the top lines */
	wp->w_linep = lp;	/* if necessary.        */
	curwp->w_flag |= WFMODE | WFHARD;
	wp->w_flag |= WFMODE | WFHARD;
	return TRUE;
}


/* Enlarge the current window. Find the window that loses space. Make sure it
 * is big enough. If so, hack the window descriptions, and ask redisplay to do
 * all the hard work. You don't just set "force reframe" because dot would
 * move. Bound to "C-X Z".
 */
BINDABLE( enlargewind) {
	window_p adjwp;
	line_p lp;
	int i;

	if (n < 0)
		return shrinkwind(f, -n);
	if( wheadp->w_wndp == NULL)
		return mloutfail( "Only one window") ;

	if ((adjwp = curwp->w_wndp) == NULL) {
		adjwp = wheadp;
		while (adjwp->w_wndp != curwp)
			adjwp = adjwp->w_wndp;
	}
	if( adjwp->w_ntrows <= n)
		return mloutfail( "Impossible change") ;

	if (curwp->w_wndp == adjwp) {	/* Shrink below.        */
		lp = adjwp->w_linep;
		for (i = 0; i < n && lp != adjwp->w_bufp->b_linep; ++i)
			lp = lforw(lp);
		adjwp->w_linep = lp;
		adjwp->w_toprow += n;
	} else {		/* Shrink above.        */
		lp = curwp->w_linep;
		for (i = 0; i < n && lback(lp) != curbp->b_linep; ++i)
			lp = lback(lp);
		curwp->w_linep = lp;
		curwp->w_toprow -= n;
	}
	curwp->w_ntrows += n;
	adjwp->w_ntrows -= n;
#if	SCROLLCODE
	curwp->w_flag |= WFMODE | WFHARD | WFINS;
	adjwp->w_flag |= WFMODE | WFHARD | WFKILLS;
#else
	curwp->w_flag |= WFMODE | WFHARD;
	adjwp->w_flag |= WFMODE | WFHARD;
#endif
	return TRUE;
}


/* Shrink the current window. Find the window that gains space. Hack at the
 * window descriptions. Ask the redisplay to do all the hard work. Bound to
 * "C-X C-Z".
 */
BINDABLE( shrinkwind) {
	window_p adjwp;
	line_p lp;
	int i;

	if (n < 0)
		return enlargewind(f, -n);
	if( wheadp->w_wndp == NULL)
		return mloutfail( "Only one window") ;

	if ((adjwp = curwp->w_wndp) == NULL) {
		adjwp = wheadp;
		while (adjwp->w_wndp != curwp)
			adjwp = adjwp->w_wndp;
	}
	if( curwp->w_ntrows <= n)
		return mloutfail( "Impossible change") ;

	if (curwp->w_wndp == adjwp) {	/* Grow below.          */
		lp = adjwp->w_linep;
		for (i = 0; i < n && lback(lp) != adjwp->w_bufp->b_linep;
		     ++i)
			lp = lback(lp);
		adjwp->w_linep = lp;
		adjwp->w_toprow -= n;
	} else {		/* Grow above.          */
		lp = curwp->w_linep;
		for (i = 0; i < n && lp != curbp->b_linep; ++i)
			lp = lforw(lp);
		curwp->w_linep = lp;
		curwp->w_toprow += n;
	}
	curwp->w_ntrows -= n;
	adjwp->w_ntrows += n;
#if	SCROLLCODE
	curwp->w_flag |= WFMODE | WFHARD | WFKILLS;
	adjwp->w_flag |= WFMODE | WFHARD | WFINS;
#else
	curwp->w_flag |= WFMODE | WFHARD;
	adjwp->w_flag |= WFMODE | WFHARD;
#endif
	return TRUE;
}


/* Resize the current window to the requested size
 *
 * int f, n;		default flag and numeric argument
 */
BINDABLE( resize) {
	int clines;		/* current # of lines in window */

	/* must have a non-default argument, else ignore call */
	if (f == FALSE)
		return TRUE;

	/* find out what to do */
	clines = curwp->w_ntrows;

	/* already the right size? */
	if (clines == n)
		return TRUE;

	return enlargewind(TRUE, n - clines);
}


/* Pick a window for a pop-up. Split the screen if there is only one window.
 * Pick the uppermost window that isn't the current window. An LRU algorithm
 * might be better. Return a pointer, or NULL on error.
 */
window_p wpopup(void)
{
	window_p wp;

	if (wheadp->w_wndp == NULL	/* Only 1 window        */
	    && splitwind(FALSE, 0) == FALSE)	/* and it won't split   */
		return NULL;
	wp = wheadp;		/* Find window to use   */
	while (wp != NULL && wp == curwp)
		wp = wp->w_wndp;
	return wp;
}


/* scroll the next window up (back) a page */
BINDABLE( scrnextup) {
	nextwind(FALSE, 1);
	backpage(f, n);
	prevwind(FALSE, 1);
	return TRUE;
}


/* scroll the next window down (forward) a page */
BINDABLE( scrnextdw) {
	nextwind(FALSE, 1);
	forwpage(f, n);
	prevwind(FALSE, 1);
	return TRUE;
}


/* save ptr to current window */
BINDABLE( savewnd) {
	savwindow = curwp ;
	return TRUE ;
}


/* restore the saved screen */
BINDABLE( restwnd) {
/* check the saved window still exists */
	for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
		if( wp == savwindow) {
			curwp = wp ;
			curbp = wp->w_bufp ;
			upmode() ;
			return TRUE ;
		}
	}

	return mloutfail( "(No such window exists)") ;
}


static void adjust( window_p wp, int screenrows) {
	wp->w_ntrows = screenrows - wp->w_toprow - 2 ;
	wp->w_flag |= WFHARD | WFMODE ;
}

/* resize the screen, re-writing the screen
 *
 * int f;	default flag
 * int n;	numeric argument
 */
BBINDABLE( newsize) {
	window_p wp ;		/* current window being examined */

	/* if the command defaults, assume the largest */
	if( f == FALSE)
		n = term.t_mrow ;

	/* make sure it's in range */
	if( n < MINROWS || n > term.t_mrow)
		return mloutfail( "%%Screen size out of range") ;

	if( term.t_nrow == n - 1)
	/* no change */
		return TRUE ;
	else if( term.t_nrow < n - 1) {
	/* new size is bigger */
		/* go to the last window */
		for( wp = wheadp ; wp->w_wndp != NULL ; wp = wp->w_wndp)
			;

		/* and enlarge it as needed */
		adjust( wp, n) ;
	} else {
	/* new size is smaller */
		/* rebuild the window structure */
		assert( wheadp->w_toprow == 0) ;	/* proves coverity wrong */
		window_p lastwp = NULL ;
		for( window_p nextwp = wheadp ; nextwp != NULL ; ) {
			wp = nextwp ;
			nextwp = wp->w_wndp ;

			/* expand previous window if current would have zero lines */
			if( wp->w_toprow == n - 2)
				adjust( lastwp, n) ;

			/* get rid of it if it is too low */
			if( wp->w_toprow >= n - 2) {
				/* save the point/mark if needed */
				if( --wp->w_bufp->b_nwnd == 0) {
					wp->w_bufp->b_dotp = wp->w_dotp ;
					wp->w_bufp->b_doto = wp->w_doto ;
					wp->w_bufp->b_markp = wp->w_markp ;
					wp->w_bufp->b_marko = wp->w_marko ;
				}

				/* update curwp and lastwp if needed */
				if( wp == curwp) {
					curwp = wheadp ;
					curbp = curwp->w_bufp ;
				}

				/* free the structure */
				free( wp) ;
				lastwp->w_wndp = NULL ;
			} else {
			/* need to change this window size? */
				if( (wp->w_toprow + wp->w_ntrows - 1) >= n - 2)
					adjust( wp, n) ;

				lastwp = wp ;
			}
		}
	}

	/* screen is garbage */
	term.t_nrow = n - 1 ;
	sgarbf = TRUE ;
	return TRUE ;
}


/* resize the screen, re-writing the screen
 *
 * int f;		default flag
 * int n;		numeric argument
 */
BBINDABLE( newwidth) {
	/* if the command defaults, assume the largest */
	if( f == FALSE)
		n = term.t_mcol ;

	/* make sure it's in range */
	if( n < MINCOLS || n > term.t_mcol)
		return mloutfail( "%%Screen width out of range") ;

	/* otherwise, just re-width it (no big deal) */
	term.t_ncol = n ;
	updmargin() ;

	/* force all windows to redraw */
	for( window_p wp = wheadp ; wp; wp = wp->w_wndp)
		wp->w_flag |= WFHARD | WFMOVE | WFMODE ;

	sgarbf = TRUE ;
	return TRUE ;
}

int getwpos(void)
{				/* get screen offset of current line in current window */
	int sline;	/* screen line from top of window */
	line_p lp;	/* scannile line pointer */

	/* search down the line we want */
	lp = curwp->w_linep;
	sline = 1;
	while (lp != curwp->w_dotp) {
		++sline;
		lp = lforw(lp);
	}

	/* and return the value */
	return sline;
}

void cknewwindow(void)
{
	execute(META | SPEC | 'X', FALSE, 1);
}

/* end of window.c */
