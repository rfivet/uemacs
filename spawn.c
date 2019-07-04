/* spawn.c -- implements spawn.h */
#include "spawn.h"

/*	spawn.c
 *
 *	Various operating system access commands.
 *
 *	<odified by Petri Kutvonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"

#include "buffer.h"
#include "display.h"
#include "estruct.h"
#include "exec.h"
#include "file.h"
#include "flook.h"
#include "input.h"
#include "terminal.h"
#include "window.h"


#if     V7 | USG | BSD
#include        <signal.h>
#ifdef SIGWINCH
#endif
#endif

#if	MSDOS & (MSC | TURBO)
#include	<process.h>
#endif


/*
 * Create a subjob with a copy of the command intrepreter in it. When the
 * command interpreter exits, mark the screen as garbage so that you do a full
 * repaint. Bound to "^X C".
 */
int spawncli(int f, int n)
{
#if     V7 | USG | BSD
	char *cp;
#endif

	/* don't allow this command if restricted */
	if (restflag)
		return resterr();

#if     MSDOS & (MSC | TURBO)
	movecursor(term.t_nrow, 0);	/* Seek to last line.   */
	TTflush();
	TTkclose();
	shellprog("");
	TTkopen();
	sgarbf = TRUE;
	return TRUE;
#endif
#if     V7 | USG | BSD
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
#endif
}

#if	BSD | __hpux | SVR4

int bktoshell(int f, int n)
{				/* suspend MicroEMACS and wait to wake up */
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

/*
 * Run a one-liner in a subjob. When the command returns, wait for a single
 * character to be typed, then mark the screen as garbage so a full repaint is
 * done. Bound to "C-X !".
 */
int spawn( int f, int n) {
	int s ;
	char *line ;

	/* don't allow this command if restricted */
	if( restflag)
		return resterr();

#if     V7 | USG | BSD
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
#endif
}

/*
 * Run an external program with arguments. When it returns, wait for a single
 * character to be typed, then mark the screen as garbage so a full repaint is
 * done. Bound to "C-X $".
 */

int execprg( int f, int n) {
	int s ;
	char *line ;

	/* don't allow this command if restricted */
	if( restflag)
		return resterr() ;

#if     V7 | USG | BSD
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
#endif
}

/*
 * Pipe a one line command into a window
 * Bound to ^X @
 */
int pipecmd( int f, int n) {
	int s ;		/* return status from CLI */
	struct window *wp ;	/* pointer to new window */
	struct buffer *bp ;	/* pointer to buffer to zot */
	char *mlarg ;
	char *line ;	/* command line send to shell */
	static char bname[] = "command" ;
	static char filnam[ NSTRING] = "command" ;

	/* don't allow this command if restricted */
	if( restflag)
		return resterr() ;

	/* get the command to pipe in */
	s = newmlarg( &mlarg, "@", 0) ;
	if( s != TRUE)
		return s ;

	line = malloc( strlen( mlarg) + strlen( filnam) + 2) ;
	if( line == NULL) {
		free( mlarg) ;
		return FALSE ;
	}

	strcpy( line, mlarg) ;
	free( mlarg) ;

	/* get rid of the command output buffer if it exists */
	if ((bp = bfind(bname, FALSE, 0)) != FALSE) {
		/* try to make sure we are off screen */
		wp = wheadp;
		while (wp != NULL) {
			if (wp->w_bufp == bp) {
#if	PKCODE
				if (wp == curwp)
					delwind(FALSE, 1);
				else
					onlywind(FALSE, 1);
				break;
#else
				onlywind(FALSE, 1);
				break;
#endif
			}
			wp = wp->w_wndp;
		}

		if( zotbuf( bp) != TRUE) {
			free( line) ;
			return FALSE ;
		}
	}
#if     V7 | USG | BSD
	TTflush();
	TTclose();		/* stty to old modes    */
	TTkclose();
	strcat( line, ">") ;
	strcat( line, filnam) ;
	ue_system( line) ;
	free( line) ;
	TTopen();
	TTkopen();
	TTflush();
	sgarbf = TRUE;
	s = TRUE;
#else
	if (s != TRUE)
		return s;
#endif

	/* split the current window to make room for the command output */
	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;

	/* and read the stuff in */
	if (getfile(filnam, FALSE) == FALSE)
		return FALSE;

	/* make this window in VIEW mode, update all mode lines */
	curwp->w_bufp->b_mode |= MDVIEW;
	wp = wheadp;
	while (wp != NULL) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}

	/* and get rid of the temporary file */
	unlink(filnam);
	return TRUE;
}

/*
 * filter a buffer through an external DOS program
 * Bound to ^X #
 */
int filter_buffer( int f, int n) {
	int s ;				/* return status from CLI */
	struct buffer *bp ;	/* pointer to buffer to zot */
	char *mlarg ;
	char *line ;		/* command line send to shell */
	fname_t tmpnam ;	/* place to store real file name */
	static char bname1[] = "fltinp" ;

	static char filnam1[] = "fltinp" ;
	static char filnam2[] = "fltout" ;

	/* don't allow this command if restricted */
	if( restflag)
		return resterr() ;

	if( curbp->b_mode & MDVIEW)	/* don't allow this command if      */
		return rdonly() ;		/* we are in read only mode     */

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

#if     V7 | USG | BSD
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


#if	MSDOS & (TURBO | MSC)

/*
 * SHELLPROG: Execute a command in a subshell
 *
 * char *cmd;		Incoming command line to execute
 */
int shellprog(char *cmd)
{
	char *shell;		/* Name of system command processor */
	char *p;		/* Temporary pointer */
	char swchar;		/* switch character to use */
	union REGS regs;	/* parameters for dos call */
	char comline[NSTRING];	/* constructed command line */

	/*  detect current switch character and set us up to use it */
	regs.h.ah = 0x37;	/*  get setting data  */
	regs.h.al = 0x00;	/*  get switch character  */
	intdos(&regs, &regs);
	swchar = (char) regs.h.dl;

	/*  get name of system shell  */
	if ((shell = getenv("COMSPEC")) == NULL) {
		return FALSE;	/*  No shell located  */
	}

	/* trim leading whitespace off the command */
	while (*cmd == ' ' || *cmd == '\t')	/*  find out if null command */
		cmd++;

	/**  If the command line is not empty, bring up the shell  **/
	/**  and execute the command.  Otherwise, bring up the     **/
	/**  shell in interactive mode.   **/

	if (*cmd) {
		strcpy(comline, shell);
		strcat(comline, " ");
		comline[strlen(comline) + 1] = 0;
		comline[strlen(comline)] = swchar;
		strcat(comline, "c ");
		strcat(comline, cmd);
		return execprog(comline);
	} else
		return execprog(shell);
}

/*
 * EXECPROG:
 *	A function to execute a named program
 *	with arguments
 *
 * char *cmd;		Incoming command line to execute
 */
int execprog(char *cmd)
{
	char *sp;		/* temporary string pointer */
	char f1[38];		/* FCB1 area (not initialized */
	char f2[38];		/* FCB2 area (not initialized */
	char prog[NSTRING];	/* program filespec */
	char tail[NSTRING];	/* command tail with length byte */
	union REGS regs;	/* parameters for dos call  */
	struct SREGS segreg;	/* segment registers for dis call */
	struct pblock {		/* EXEC parameter block */
		short envptr;	/* 2 byte pointer to environment string */
		char *cline;	/* 4 byte pointer to command line */
		char *fcb1;	/* 4 byte pointer to FCB at PSP+5Ch */
		char *fcb2;	/* 4 byte pointer to FCB at PSP+6Ch */
	} pblock;

	/* parse the command name from the command line */
	sp = prog;
	while (*cmd && (*cmd != ' ') && (*cmd != '\t'))
		*sp++ = *cmd++;
	*sp = 0;

	/* and parse out the command tail */
	while (*cmd && ((*cmd == ' ') || (*cmd == '\t')))
		++cmd;
	*tail = (char) (strlen(cmd));	/* record the byte length */
	strcpy(&tail[1], cmd);
	strcat(&tail[1], "\r");

	/* look up the program on the path trying various extentions */
	if ((sp = flook(prog, TRUE)) == NULL)
		if ((sp = flook(strcat(prog, ".exe"), TRUE)) == NULL) {
			strcpy(&prog[strlen(prog) - 4], ".com");
			if ((sp = flook(prog, TRUE)) == NULL)
				return FALSE;
		}
	strcpy(prog, sp);

	/* get a pointer to this PSPs environment segment number */
	segread(&segreg);

	/* set up the EXEC parameter block */
	pblock.envptr = 0;	/* make the child inherit the parents env */
	pblock.fcb1 = f1;	/* point to a blank FCB */
	pblock.fcb2 = f2;	/* point to a blank FCB */
	pblock.cline = tail;	/* parameter line pointer */

	/* and make the call */
	regs.h.ah = 0x4b;	/* EXEC Load or Execute a Program */
	regs.h.al = 0x00;	/* load end execute function subcode */
	segreg.ds = ((unsigned long) (prog) >> 16);	/* program name ptr */
	regs.x.dx = (unsigned int) (prog);
	segreg.es = ((unsigned long) (&pblock) >> 16);	/* set up param block ptr */
	regs.x.bx = (unsigned int) (&pblock);
#if	TURBO | MSC
	intdosx(&regs, &regs, &segreg);
	if (regs.x.cflag == 0) {
		regs.h.ah = 0x4d;	/* get child process return code */
		intdos(&regs, &regs);	/* go do it */
		rval = regs.x.ax;	/* save child's return code */
	} else
#if	MSC
		rval = -1;
#else
		rval = -_doserrno;	/* failed child call */
#endif
#endif
	return (rval < 0) ? FALSE : TRUE;
}
#endif
