/* basic.c -- implements basic.h */

#include "basic.h"

/*	basic.c
 *
 * The routines in this file move the cursor around on the screen. They
 * compute a new value for the cursor, then adjust ".". The display code
 * always updates the cursor location, so only moves between lines, or
 * functions that adjust the top line in the window and invalidate the
 * framing, are hard.
 *
 *	modified by Petri Kutvonen
 */

#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"
#include "display.h"
#include "estruct.h"
#include "input.h"
#include "line.h"
#include "random.h"
#include "terminal.h"
#include "utf8.h"
#include "window.h"


int overlap = 0 ;		/* line overlap in forw/back page	*/
int curgoal ;			/* Goal for C-P, C-N			*/


/*
 * This routine, given a pointer to a struct line, and the current cursor goal
 * column, return the best choice for the offset. The offset is returned.
 * Used by "C-N" and "C-P".
 */
static int getgoal(struct line *dlp)
{
	int col;
	int newcol;
	int dbo;
	int len = llength(dlp);

	col = 0;
	dbo = 0;
	while (dbo != len) {
		unicode_t c;
		int width = utf8_to_unicode(dlp->l_text, dbo, len, &c);
		newcol = col;

		/* Take tabs, ^X and \xx hex characters into account */
		if (c == '\t')
			newcol |= tabmask;
		else if (c < 0x20 || c == 0x7F)
			++newcol;
		else if (c >= 0x80 && c <= 0xa0)
			newcol += 2;

		++newcol;
		if (newcol > curgoal)
			break;
		col = newcol;
		dbo += width;
	}
	return dbo;
}

/*
 * Move the cursor to the beginning of the current line.
 */
int gotobol(int f, int n)
{
	curwp->w_doto = 0;
	return TRUE;
}

/*
 * Move the cursor to the end of the current line. Trivial. No errors.
 */
int gotoeol(int f, int n)
{
	curwp->w_doto = llength(curwp->w_dotp);
	return TRUE;
}

/*
 * Move to a particular line.
 *
 * @n: The specified line position at the current buffer.
 */
int gotoline(int f, int n)
{
	int status;
	char arg[NSTRING]; /* Buffer to hold argument. */

	/* Get an argument if one doesnt exist. */
	if (f == FALSE) {
		if ((status =
		     mlreply("Line to GOTO: ", arg, NSTRING)) != TRUE) {
			mlwrite("(Aborted)");
			return status;
		}
		n = atoi(arg);
	}
        /* Handle the case where the user may be passed something like this:
         * em filename +
         * In this case we just go to the end of the buffer.
         */
	if (n == 0)
		return gotoeob(f, n);

	/* If a bogus argument was passed, then returns false. */
	if (n < 0)
		return FALSE;

	/* First, we go to the begin of the buffer. */
	gotobob(f, n);
	return forwline(f, n - 1);
}

/*
 * Goto the beginning of the buffer. Massive adjustment of dot. This is
 * considered to be hard motion; it really isn't if the original value of dot
 * is the same as the new value of dot. Normally bound to "M-<".
 */
int gotobob(int f, int n)
{
	curwp->w_dotp = lforw(curbp->b_linep);
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/*
 * Move to the end of the buffer. Dot is always put at the end of the file
 * (ZJ). The standard screen code does most of the hard parts of update.
 * Bound to "M->".
 */
int gotoeob(int f, int n)
{
	curwp->w_dotp = curbp->b_linep;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/*
 * Move forward by full lines. If the number of lines to move is less than
 * zero, call the backward line function to actually do it. The last command
 * controls how the goal column is set. Bound to "C-N". No errors are
 * possible.
 */
int forwline(int f, int n)
{
	struct line *dlp;

	if (n < 0)
		return backline(f, -n);

	/* if we are on the last line as we start....fail the command */
	if (curwp->w_dotp == curbp->b_linep)
		return FALSE;

	/* if the last command was not note a line move,
	   reset the goal column */
	if ((lastflag & CFCPCN) == 0)
		curgoal = getccol(FALSE);

	/* flag this command as a line move */
	thisflag |= CFCPCN;

	/* and move the point down */
	dlp = curwp->w_dotp;
	while (n-- && dlp != curbp->b_linep)
		dlp = lforw(dlp);

	/* reseting the current position */
	curwp->w_dotp = dlp;
	curwp->w_doto = getgoal(dlp);
	curwp->w_flag |= WFMOVE;
	return TRUE;
}

/*
 * This function is like "forwline", but goes backwards. The scheme is exactly
 * the same. Check for arguments that are less than zero and call your
 * alternate. Figure out the new line and call "movedot" to perform the
 * motion. No errors are possible. Bound to "C-P".
 */
int backline(int f, int n)
{
	struct line *dlp;

	if (n < 0)
		return forwline(f, -n);

	/* if we are on the last line as we start....fail the command */
	if (lback(curwp->w_dotp) == curbp->b_linep)
		return FALSE;

	/* if the last command was not note a line move,
	   reset the goal column */
	if ((lastflag & CFCPCN) == 0)
		curgoal = getccol(FALSE);

	/* flag this command as a line move */
	thisflag |= CFCPCN;

	/* and move the point up */
	dlp = curwp->w_dotp;
	while (n-- && lback(dlp) != curbp->b_linep)
		dlp = lback(dlp);

	/* reseting the current position */
	curwp->w_dotp = dlp;
	curwp->w_doto = getgoal(dlp);
	curwp->w_flag |= WFMOVE;
	return TRUE;
}

/*
 * Scroll forward by a specified number of lines, or by a full page if no
 * argument. Bound to "C-V". The "2" in the arithmetic on the window size is
 * the overlap; this value is the default overlap value in ITS EMACS. Because
 * this zaps the top line in the display window, we have to do a hard update.
 */
int forwpage(int f, int n)
{
	struct line *lp;

	if (f == FALSE) {
#if SCROLLCODE
		if (term.t_scroll != NULL)
			if (overlap == 0)
				n = curwp->w_ntrows / 3 * 2;
			else
				n = curwp->w_ntrows - overlap;
		else
#endif
			n = curwp->w_ntrows - 2;  /* Default scroll. */
		if (n <= 0)	/* Forget the overlap. */
			n = 1;	/* If tiny window. */
	} else if (n < 0)
		return backpage(f, -n);
#if     CVMVAS
	else			/* Convert from pages. */
		n *= curwp->w_ntrows;	/* To lines. */
#endif
	lp = curwp->w_linep;
	while (n-- && lp != curbp->b_linep)
		lp = lforw(lp);
	curwp->w_linep = lp;
	curwp->w_dotp = lp;
	curwp->w_doto = 0;
#if SCROLLCODE
	curwp->w_flag |= WFHARD | WFKILLS;
#else
	curwp->w_flag |= WFHARD;
#endif
	return TRUE;
}

/*
 * This command is like "forwpage", but it goes backwards. The "2", like
 * above, is the overlap between the two windows. The value is from the ITS
 * EMACS manual. Bound to "M-V". We do a hard update for exactly the same
 * reason.
 */
int backpage(int f, int n)
{
	struct line *lp;

	if (f == FALSE) {
#if SCROLLCODE
		if (term.t_scroll != NULL)
			if (overlap == 0)
				n = curwp->w_ntrows / 3 * 2;
			else
				n = curwp->w_ntrows - overlap;
		else
#endif
			n = curwp->w_ntrows - 2; /* Default scroll. */
		if (n <= 0)	/* Don't blow up if the. */
			n = 1;	/* Window is tiny. */
	} else if (n < 0)
		return forwpage(f, -n);
#if     CVMVAS
	else  /* Convert from pages. */
		n *= curwp->w_ntrows;  /* To lines. */
#endif
	lp = curwp->w_linep;
	while (n-- && lback(lp) != curbp->b_linep)
		lp = lback(lp);
	curwp->w_linep = lp;
	curwp->w_dotp = lp;
	curwp->w_doto = 0;
#if SCROLLCODE
	curwp->w_flag |= WFHARD | WFINS;
#else
	curwp->w_flag |= WFHARD;
#endif
	return TRUE;
}

/*
 * Set the mark in the current window to the value of "." in the window. No
 * errors are possible. Bound to "M-.".
 */
int setmark(int f, int n)
{
	curwp->w_markp = curwp->w_dotp;
	curwp->w_marko = curwp->w_doto;
	mlwrite("(Mark set)");
	return TRUE;
}

/*
 * Swap the values of "." and "mark" in the current window. This is pretty
 * easy, bacause all of the hard work gets done by the standard routine
 * that moves the mark about. The only possible error is "no mark". Bound to
 * "C-X C-X".
 */
int swapmark(int f, int n)
{
	struct line *odotp;
	int odoto;

	if (curwp->w_markp == NULL) {
		mlwrite("No mark in this window");
		return FALSE;
	}
	odotp = curwp->w_dotp;
	odoto = curwp->w_doto;
	curwp->w_dotp = curwp->w_markp;
	curwp->w_doto = curwp->w_marko;
	curwp->w_markp = odotp;
	curwp->w_marko = odoto;
	curwp->w_flag |= WFMOVE;
	return TRUE;
}
