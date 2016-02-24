/* execute.c -- implements execute.h */
#include "execute.h"

#include <stdlib.h>

#include "estruct.h"
#include "bind.h"
#include "random.h"
#include "display.h"
#include "file.h"
#include "mlout.h"
#include "window.h"

int gasave = 256 ;		/* global ASAVE size            */
int gacount = 256 ;		/* count until next ASAVE       */

/*
 * This is the general command execution routine. It handles the fake binding
 * of all the keys to "self-insert". It also clears out the "thisflag" word,
 * and arranges to move it to the "lastflag", so that the next command can
 * look at it. Return the status of command.
 */
int execute(int c, int f, int n)
{
	int status;
	fn_t execfunc;

	/* if the keystroke is a bound function...do it */
	execfunc = getbind(c);
	if (execfunc != NULL) {
		thisflag = 0;
		status = (*execfunc) (f, n);
		lastflag = thisflag;
		return status;
	}

	/*
	 * If a space was typed, fill column is defined, the argument is non-
	 * negative, wrap mode is enabled, and we are now past fill column,
	 * and we are not read-only, perform word wrap.
	 */
	if (c == ' ' && (curwp->w_bufp->b_mode & MDWRAP) && fillcol > 0 &&
	    n >= 0 && getccol(FALSE) > fillcol &&
	    (curwp->w_bufp->b_mode & MDVIEW) == FALSE)
		execute(META | SPEC | 'W', FALSE, 1);

#if	PKCODE
	if ((c >= 0x20 && c <= 0x7E)	/* Self inserting.      */
#if	IBMPC
	    || (c >= 0x80 && c <= 0xFE)) {
#else
#if	VMS || BSD || USG	/* 8BIT P.K. */
	    || (c >= 0xA0 && c <= 0x10FFFF)) {
#else
	    ) {
#endif
#endif
#else
	if ((c >= 0x20 && c <= 0xFF)) {	/* Self inserting.      */
#endif
		if (n <= 0) {	/* Fenceposts.          */
			lastflag = 0;
			return n < 0 ? FALSE : TRUE;
		}
		thisflag = 0;	/* For the future.      */

		/* if we are in overwrite mode, not at eol,
		   and next char is not a tab or we are at a tab stop,
		   delete a char forword                        */
		if (curwp->w_bufp->b_mode & MDOVER &&
		    curwp->w_doto < curwp->w_dotp->l_used &&
		    (lgetc(curwp->w_dotp, curwp->w_doto) != '\t' ||
		     ((curwp->w_doto) % tabwidth) == (tabwidth - 1)))
			ldelchar(1, FALSE);

		/* do the appropriate insertion */
		if (c == '}' && (curbp->b_mode & MDCMOD) != 0)
			status = insbrace(n, c);
		else if (c == '#' && (curbp->b_mode & MDCMOD) != 0)
			status = inspound();
		else
			status = linsert(n, c);

#if	CFENCE
		/* check for CMODE fence matching */
		if ((c == '}' || c == ')' || c == ']') &&
		    (curbp->b_mode & MDCMOD) != 0)
			fmatch(c);
#endif

		/* check auto-save mode */
		if (curbp->b_mode & MDASAVE)
			if (--gacount == 0) {
				/* and save the file if needed */
				upscreen(FALSE, 0);
				filesave(FALSE, 0);
				gacount = gasave;
			}

		lastflag = thisflag;
		return status;
	}

	lastflag = 0 ;		/* Fake last flags. */
	mloutfmt( "%B(Key not bound)") ;	/* Complain */
	return FALSE ;
}

