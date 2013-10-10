/* bindable.h -- implements bindable.c */
#include "bindable.h"

#include <stdlib.h>


#include "defines.h"
#include "buffer.h"
#include "display.h"
#include "estruct.h"
#include "file.h"
#include "input.h"
#include "lock.h"
#include "terminal.h"

#if VMS
#include <ssdef.h>
#define GOOD    (SS$_NORMAL)
#endif

#ifndef GOOD
#define GOOD    0
#endif

/*
 * Fancy quit command, as implemented by Norm. If the any buffer has
 * changed do a write on that buffer and exit emacs, otherwise simply exit.
 */
int quickexit(int f, int n)
{
	struct buffer *bp;	/* scanning pointer to buffers */
	struct buffer *oldcb;	/* original current buffer */
	int status;

	oldcb = curbp;		/* save in case we fail */

	bp = bheadp;
	while (bp != NULL) {
		if ((bp->b_flag & BFCHG) != 0	/* Changed.             */
		    && (bp->b_flag & BFTRUNC) == 0	/* Not truncated P.K.   */
		    && (bp->b_flag & BFINVS) == 0) {	/* Real.                */
			curbp = bp;	/* make that buffer cur */
			mlwrite("(Saving %s)", bp->b_fname);
#if	PKCODE
#else
			mlwrite("\n");
#endif
			if ((status = filesave(f, n)) != TRUE) {
				curbp = oldcb;	/* restore curbp */
				return status;
			}
		}
		bp = bp->b_bufp;	/* on to the next buffer */
	}
	quit(f, n);		/* conditionally quit   */
	return TRUE;
}

/*
 * Quit command. If an argument, always quit. Otherwise confirm if a buffer
 * has been changed and not written out. Normally bound to "C-X C-C".
 */
int quit(int f, int n)
{
	int s;

	if (f != FALSE		/* Argument forces it.  */
	    || anycb() == FALSE	/* All buffers clean.   */
	    /* User says it's OK.   */
	    || (s =
		mlyesno("Modified buffers exist. Leave anyway")) == TRUE) {
#if	(FILOCK && BSD) || SVR4
		if (lockrel() != TRUE) {
			TTputc('\n');
			TTputc('\r');
			TTclose();
			TTkclose();
			exit(1);
		}
#endif
		vttidy();
		if (f)
			exit(n);
		else
			exit(GOOD);
	}
	mlwrite("");
	return s;
}

/*
 * Begin a keyboard macro.
 * Error if not at the top level in keyboard processing. Set up variables and
 * return.
 */
int ctlxlp(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite("%%Macro already active");
		return FALSE;
	}
	mlwrite("(Start macro)");
	kbdptr = &kbdm[0];
	kbdend = kbdptr;
	kbdmode = RECORD;
	return TRUE;
}

/*
 * End keyboard macro. Check for the same limit conditions as the above
 * routine. Set up the variables and return to the caller.
 */
int ctlxrp(int f, int n)
{
	if (kbdmode == STOP) {
		mlwrite("%%Macro not active");
		return FALSE;
	}
	if (kbdmode == RECORD) {
		mlwrite("(End macro)");
		kbdmode = STOP;
	}
	return TRUE;
}

/*
 * Execute a macro.
 * The command argument is the number of times to loop. Quit as soon as a
 * command gets an error. Return TRUE if all ok, else FALSE.
 */
int ctlxe(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite("%%Macro already active");
		return FALSE;
	}
	if (n <= 0)
		return TRUE;
	kbdrep = n;		/* remember how many times to execute */
	kbdmode = PLAY;		/* start us in play mode */
	kbdptr = &kbdm[0];	/*    at the beginning */
	return TRUE;
}

/*
 * Abort.
 * Beep the beeper. Kill off any keyboard macro, etc., that is in progress.
 * Sometimes called as a routine, to do general aborting of stuff.
 */
int ctrlg(int f, int n)
{
	TTbeep();
	kbdmode = STOP;
	mlwrite("(Aborted)");
	return ABORT;
}

/* user function that does NOTHING */
int nullproc(int f, int n)
{
	return TRUE;
}

/* dummy function for binding to meta prefix */
int metafn(int f, int n)
{
	return TRUE;
}

/* dummy function for binding to control-x prefix */
int cex(int f, int n)
{
	return TRUE;
}

/* dummy function for binding to universal-argument */
int unarg(int f, int n)
{
	return TRUE;
}
