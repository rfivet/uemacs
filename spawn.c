/* spawn.c -- implements spawn.h */
#include "spawn.h"

/*	Various operating system access commands.
 *
 *	Modified by Petri Kutvonen
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "defines.h"
#include "display.h"
#include "exec.h"
#include "file.h"
#include "flook.h"
#include "input.h"
#include "terminal.h"
#include "window.h"


#if	USG | BSD
#include        <signal.h>
#endif


/* Create a subjob with a copy of the command intrepreter in it. When the
 * command interpreter exits, mark the screen as garbage so that you do a full
 * repaint. Bound to "^X C".
 */
BINDABLE( spawncli) {
#if	USG | BSD
	char *cp;
#endif

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

#if	USG | BSD
	movecursor(term.t_nrow, 0);	/* Seek to last line.   */
	TTflush();
	TTclose();		/* stty to old settings */
	TTkclose();		/* Close "keyboard" */
	if ((cp = getenv("SHELL")) != NULL && *cp != '\0')
		ue_system( cp) ;
	else
#if	BSD
		system("exec /bin/csh");
#else
		ue_system( "exec /bin/sh") ;
#endif
	sgarbf = TRUE;
	usleep( 2000000L) ;
	TTopen();
	TTkopen();
#ifdef SIGWINCH
/*
 * This fools the update routines to force a full
 * redraw with complete window size checking.
 *		-lbt
 */
	chg_width = term.t_ncol;
	chg_height = term.t_nrow + 1;
	term.t_nrow = term.t_ncol = 0;
#endif
	return TRUE;
#else
        return FALSE;
#endif
}

#if	BSD | SVR4
/* suspend MicroEMACS and wait to wake up */
BINDABLE( bktoshell) {
	vttidy();
/******************************
	int pid;

	pid = getpid();
	kill(pid,SIGTSTP);
******************************/
	kill(0, SIGTSTP);
	return TRUE;
}

void rtfrmshell(void)
{
	TTopen();
	curwp->w_flag = WFHARD;
	sgarbf = TRUE;
}
#endif


/* Run a one-liner in a subjob. When the command returns, wait for a single
 * character to be typed, then mark the screen as garbage so a full repaint is
 * done. Bound to "C-X !".
 */
BINDABLE( spawn) {

	/* don't allow this command if restricted */
	if( restflag)
		return resterr();

#if	USG | BSD
	int s ;
	char *line ;
	s = newmlarg( &line, "!", 0) ;
	if( s != TRUE)
		return s ;

	TTflush();
	TTclose();		/* stty to old modes    */
	TTkclose();
	ue_system( line) ;
	free( line) ;
	fflush(stdout);		/* to be sure P.K.      */
	TTopen();

	if (clexec == FALSE) {
		mlwrite( "(End)") ;	/* Pause.               */
		TTflush();
		while ((s = tgetc()) != '\r' && s != ' ');
		mlwrite( "\r\n") ;
	}
	TTkopen();
	sgarbf = TRUE;
	return TRUE;
#else
	return FALSE;
#endif
}


/* Run an external program with arguments. When it returns, wait for a single
 * character to be typed, then mark the screen as garbage so a full repaint is
 * done. Bound to "C-X $".
 */

BINDABLE( execprg) {

	/* don't allow this command if restricted */
	if( restflag)
		return resterr() ;

#if	USG | BSD
	int s ;
	char *line ;
	s = newmlarg( &line, "$", 0) ;
	if( s != TRUE)
		return s ;

	TTputc('\n');		/* Already have '\r'    */
	TTflush();
	TTclose();		/* stty to old modes    */
	TTkclose();
	ue_system( line) ;
	free( line) ;
	fflush(stdout);		/* to be sure P.K.      */
	TTopen();
	mlwrite( "(End)") ;	/* Pause.               */
	TTflush();
	while ((s = tgetc()) != '\r' && s != ' ');
	sgarbf = TRUE;
	return TRUE;
#else
	return FALSE;
#endif
}


/* Pipe a one line command into a window
 * Bound to pipe-command ^X @
 */
BINDABLE( pipecmd) {
	window_p wp ;	/* pointer to new window */
	char *mlarg ;
	const char filnam[] = "command" ;

/* don't allow this command if restricted */
	if( restflag)
		return resterr() ;

/* get the command to pipe in */
	int s = newmlarg( &mlarg, "pipe-command: ", 0) ;
	if( s != TRUE)
		return s ;

	char *cmdline = malloc( strlen( mlarg) + strlen( filnam) + 4) ;
	if( cmdline == NULL) {
		free( mlarg) ;
		return FALSE ;
	}

	strcpy( cmdline, mlarg) ;
	free( mlarg) ;
	strcat( cmdline, " > ") ;
	strcat( cmdline, filnam) ;

/* get rid of the command output buffer if it exists */
	buffer_p bp = bfind( filnam, FALSE, 0) ;
	if( bp != NULL) {
	/* try to make sure we are off screen */
		for( wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
			if( wp->w_bufp == bp) {
#if	PKCODE
				if( wp == curwp)
					delwind( FALSE, 1) ;
				else
					onlywind( FALSE, 1) ;
				break ;
#else
				onlywind( FALSE, 1) ;
				break ;
#endif
			}
		}

		if( zotbuf( bp) != TRUE) {
			free( cmdline) ;
			return FALSE ;
		}
	}

#if	USG | BSD
	TTflush();
	TTclose();		/* stty to old modes    */
	TTkclose();
	ue_system( cmdline) ;
	free( cmdline) ;
	TTopen();
	TTkopen();
	TTflush();
	sgarbf = TRUE;
	s = TRUE;
#else
	if( s != TRUE)
		return s;
#endif

/* split the current window to make room for the command output
** and read the stuff in */
	if( splitwind( FALSE, 1) == FALSE
	||  getfile( filnam, FALSE) == FALSE)
		return FALSE ;

/* make this window in VIEW mode, update all mode lines */
	curwp->w_bufp->b_mode |= MDVIEW ;
	for( wp = wheadp ; wp != NULL ; wp = wp->w_wndp)
		wp->w_flag |= WFMODE ;

/* and get rid of the temporary file */
	unlink( filnam) ;
	return TRUE ;
}


/* filter a buffer through an external DOS program
 * Bound to ^X #
 */
BINDABLE( filter_buffer) {
	int s ;				/* return status from CLI */
	buffer_p bp ;		/* pointer to buffer to zot */
	char *mlarg ;
	char *line ;		/* command line send to shell */
	fname_t tmpnam ;	/* place to store real file name */
	static char bname1[] = "fltinp" ;

	static char filnam1[] = "fltinp" ;
	static char filnam2[] = "fltout" ;

	/* don't allow this command if restricted */
	if( restflag)
		return resterr() ;

	assert( !(curbp->b_mode & MDVIEW)) ;

	/* get the filter name and its args */
	s = newmlarg( &mlarg, "#", 0) ;
	if( s != TRUE)
		return s ;

	line = malloc( strlen( mlarg) + 16 + 1) ;
	if( line == NULL) {
		free( mlarg) ;
		return FALSE ;
	}

	strcpy( line, mlarg) ;
	free( mlarg) ;

	/* setup the proper file names */
	bp = curbp;
	strcpy(tmpnam, bp->b_fname);	/* save the original name */
	strcpy(bp->b_fname, bname1);	/* set it to our new one */

	/* write it out, checking for errors */
	if( writeout( filnam1) != TRUE) {
		mlwrite( "(Cannot write filter file)") ;
		strcpy( bp->b_fname, tmpnam) ;
		free( line) ;
		return FALSE ;
	}

#if	USG | BSD
	TTputc('\n');		/* Already have '\r'    */
	TTflush();
	TTclose();		/* stty to old modes    */
	TTkclose();
	strcat(line, " <fltinp >fltout");
	ue_system( line) ;
	free( line) ;
	TTopen();
	TTkopen();
	TTflush();
	sgarbf = TRUE;
	s = TRUE;
#endif

	/* on failure, escape gracefully */
	if (s != TRUE || (readin(filnam2, FALSE) == FALSE)) {
		mlwrite( "(Execution failed)") ;
		strcpy(bp->b_fname, tmpnam);
		unlink(filnam1);
		unlink(filnam2);
		return s;
	}

	/* reset file name */
	strcpy(bp->b_fname, tmpnam);	/* restore name */
	bp->b_flag |= BFCHG;	/* flag it as changed */

	/* and get rid of the temporary file */
	unlink(filnam1);
	unlink(filnam2);
	return TRUE;
}

/* end of spawn.c */
