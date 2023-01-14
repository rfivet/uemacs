/* main.c -- */

/*	µEMACS 4.2
 *
 *	Based on:
 *
 *	uEmacs/PK 4.0
 *
 *	Based on:
 *
 *	MicroEMACS 3.9
 *	Written by Dave G. Conroy.
 *	Substantially modified by Daniel M. Lawrence
 *	Modified by Petri Kutvonen
 *
 *	MicroEMACS 3.9 (c) Copyright 1987 by Daniel M. Lawrence
 *
 *	Original statement of copying policy:
 *
 *	MicroEMACS 3.9 can be copied and distributed freely for any
 *	non-commercial purposes. MicroEMACS 3.9 can only be incorporated
 *	into commercial software with the permission of the current author.
 *
 *	No copyright claimed for modifications made by Petri Kutvonen.
 *
 *	This file contains the main driving routine, and some keyboard
 *	processing code.
 *
 * REVISION HISTORY:
 *
 * 1.0  Steve Wilhite, 30-Nov-85
 *
 * 2.0  George Jones, 12-Dec-85
 *
 * 3.0  Daniel Lawrence, 29-Dec-85
 *
 * 3.2-3.6 Daniel Lawrence, Feb...Apr-86
 *
 * 3.7	Daniel Lawrence, 14-May-86
 *
 * 3.8	Daniel Lawrence, 18-Jan-87
 *
 * 3.9	Daniel Lawrence, 16-Jul-87
 *
 * 3.9e	Daniel Lawrence, 16-Nov-87
 *
 * After that versions 3.X and Daniel Lawrence went their own ways.
 * A modified 3.9e/PK was heavily used at the University of Helsinki
 * for several years on different UNIX, VMS, and MSDOS platforms.
 *
 * This modified version is now called eEmacs/PK.
 *
 * 4.0	Petri Kutvonen, 1-Sep-91
 *
 * This modified version is now called uEMACS.
 *
 * 4.1	Renaud Fivet, 1-May-13
 *
 * Renamed as µEMACS to emphasize UTF-8 support.
 *
 * 4.2	Renaud Fivet, 2015-02-12
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"	/* OS specific customization */
#if UNIX
# include <signal.h>
# include <unistd.h>
#endif

#include "basic.h"
#include "bind.h"
#include "bindable.h"
#include "buffer.h"
#include "display.h"
#include "eval.h"
#include "execute.h"
#include "file.h"
#include "lock.h"
#include "mlout.h"
#include "random.h"
#include "search.h"
#include "terminal.h"
#include "termio.h"
#include "util.h"
#include "version.h"
#include "window.h"

#if UNIX
static void emergencyexit( int signr) {
	quickexit( FALSE, 0) ;
	quit( TRUE, 0) ; /* If quickexit fails (to save changes), do a force quit */
}
#endif

static void edinit( char *bname) ;

static void version( void) {
    fputs( PROGRAM_NAME_UTF8 " version " VERSION "\n", stdout) ;
}


static void usage( void) {
    fputs( "Usage: " PROGRAM_NAME " [OPTION|FILE]..\n\n"
           "      +             start at the end of file\n"
           "      +<n>          start at line <n>\n"
           "      --help        display this help and exit\n"
           "      --version     output version information and exit\n"
           "      -a|A          process error file\n"
           "      -e|E          edit file\n"
           "      -g|G<n>       go to line <n>\n"
           "      -r|R          restrictive use\n"
           "      -s|S<string>  search string\n"
           "      -v|V          view file\n"
           "      -x|Xcmdfile\n"
           "      -x|X cmdfile  execute command file\n"
           "      @cmdfile      execute startup file\n"
           , stdout) ;
}


int main( int argc, char *argv[]) {
	buffer_p bp;	/* temp buffer pointer */
	int firstfile;	/* first file flag */
	int carg;	/* current arg to scan */
	int startflag;	/* startup executed flag */
	buffer_p firstbp = NULL;	/* ptr to first buffer in cmd line */
	int viewflag;		/* are we starting in view mode? */
	int gotoflag;		/* do we need to goto a line at start? */
	int gline = 0;		/* if so, what line? */
	int searchflag;		/* Do we need to search at start? */
	int errflag;		/* C error processing? */
	bname_t bname ;	/* buffer name of file to read */

#if	PKCODE & BSD
	sleep(1); /* Time for window manager. */
#endif

	if( argc == 2) {
		if( strcmp( argv[ 1], "--help") == 0) {
			usage() ;
			exit( EXIT_SUCCESS) ;
		}

		if( strcmp( argv[ 1], "--version") == 0) {
			version() ;
			exit( EXIT_SUCCESS) ;
		}
	}

	vtinit() ;			/* Display */
	mloutfmt = mlwrite ;
	edinit( "main") ;	/* Bindings, buffers, windows */
	varinit() ;			/* user variables */

	viewflag = FALSE;	/* view mode defaults off in command line */
	gotoflag = FALSE;	/* set to off to begin with */
	searchflag = FALSE;	/* set to off to begin with */
	firstfile = TRUE;	/* no file to edit yet */
	startflag = FALSE;	/* startup file not executed yet */
	errflag = FALSE;	/* not doing C error parsing */

	/* Insure screen is initialized before startup and goto/search */
	update( FALSE) ;

	/* Parse the command line */
	for (carg = 1; carg < argc; ++carg) {
		/* Process Switches */
#if	PKCODE
		if (argv[carg][0] == '+') {
			gotoflag = TRUE;
			gline = atoi(&argv[carg][1]);
		} else
#endif
		if (argv[carg][0] == '-') {
			switch (argv[carg][1]) {
				/* Process Startup macroes */
			case 'a':	/* process error file */
			case 'A':
				errflag = TRUE;
				break;
			case 'e':	/* -e for Edit file */
			case 'E':
				viewflag = FALSE;
				break;
			case 'g':	/* -g for initial goto */
			case 'G':
				gotoflag = TRUE;
				gline = atoi(&argv[carg][2]);
				break;
			case 'r':	/* -r restrictive use */
			case 'R':
				restflag = TRUE;
				break;
			case 's':	/* -s for initial search string */
			case 'S':
				searchflag = TRUE;
				mystrscpy( pat, &argv[ carg][ 2], sizeof pat) ;
				break;
			case 'v':	/* -v for View File */
			case 'V':
				viewflag = TRUE;
				break;
			case 'x':
			case 'X':
				if( argv[ carg][ 2]) {			/* -Xfilename */
					if( startup( &argv[ carg][ 2]) == TRUE)
						startflag = TRUE ;	/* don't execute emacs.rc */
				} else if( argv[ carg + 1]) {	/* -X filename */
					if( startup( &argv[ carg + 1][ 0]) == TRUE)
						startflag = TRUE ;	/* don't execute emacs.rc */

					carg += 1 ;
				}

				break ;
			default:	/* unknown switch */
				/* ignore this for now */
				break;
			}

		} else if (argv[carg][0] == '@') {

			/* Process Startup macroes */
			if (startup(&argv[carg][1]) == TRUE)
				/* don't execute emacs.rc */
				startflag = TRUE;

		} else {

			/* Process an input file */

			/* set up a buffer for this file */
			makename(bname, argv[carg]);
			unqname(bname);

			/* set this to inactive */
			bp = bfind( bname, TRUE, 0) ;
			if( bp == NULL) {
				fputs( "Buffer creation failed!\n", stderr) ;
				exit( EXIT_FAILURE) ;
			}

			mystrscpy( bp->b_fname, argv[ carg], sizeof bp->b_fname) ; /* max filename length limited to NFILEN - 1 */
			bp->b_active = FALSE;
			if (firstfile) {
				firstbp = bp;
				firstfile = FALSE;
			}

			/* set the modes appropriatly */
			if (viewflag)
				bp->b_mode |= MDVIEW;
		}
	}

#if	UNIX
#ifdef SIGHUP
	signal(SIGHUP, emergencyexit);
#endif
	signal(SIGTERM, emergencyexit);
#endif

	/* if we are C error parsing... run it! */
	if (errflag) {
		if (startup("error.cmd") == TRUE)
			startflag = TRUE;
	}

	/* if invoked with no other startup files,
	   run the system startup file here */
	if( (startflag == FALSE) && (startup( "") != TRUE))
		mloutstr( "Default startup failed!") ;

	discmd = TRUE;		/* P.K. */

	/* if there are any files to read, read the first one! */
	bp = bfind("main", FALSE, 0);
	if( bp == NULL) {
	/* "main" buffer has been created during early initialisation */
		fputs( "Initialisation failure!\n", stderr) ;
		exit( EXIT_FAILURE) ;
	}

	if (firstfile == FALSE && readfirst_f()) {
		swbuffer(firstbp);
		zotbuf(bp);
	} else {
		bp->b_mode |= gmode;
		upmode() ;
	}

	/* Deal with startup gotos and searches */
	if( gotoflag && searchflag)
		mloutstr( "(Can not search and goto at the same time!)") ;
	else if( gotoflag) {
		if( gotoline( TRUE, gline) == FALSE)
			mloutstr( "(Bogus goto argument)") ;
	} else if( searchflag)
		if( forwhunt( FALSE, 0))
			mloutfmt( "Found on line %d", getcline()) ;

	kbd_loop() ;
	return EXIT_SUCCESS ;	/* never reached */
}


/*
 * Initialize all of the buffers and windows. The buffer name is passed down
 * as an argument, because the main routine may have been told to read in a
 * file by default, and we want the buffer name to be right.
 */
static void edinit( char *bname) {
	buffer_p bp;
	window_p wp;

	if( !init_bindings()	/* initialize mapping of function to name and key */
	||  NULL == (bp = bfind( bname, TRUE, 0))				/* First buffer */
	||	NULL == (blistp = bfind( "*List*", TRUE, BFINVS))	/* Buffer list */
	||	NULL == (wp = malloc( sizeof *wp))) {				/* First window */
		fputs( "First initialisation failed!\n", stderr) ;
		exit( EXIT_FAILURE) ;
	}

	curbp = bp;		/* Make this current    */
	wheadp = wp;
	curwp = wp;
	wp->w_wndp = NULL;	/* Initialize window    */
	wp->w_bufp = bp;
	bp->b_nwnd = 1;		/* Displayed.           */
	wp->w_linep = bp->b_linep;
	wp->w_dotp = bp->b_linep;
	wp->w_doto = 0;
	wp->w_markp = NULL;
	wp->w_marko = 0;
	wp->w_toprow = 0;
#if	COLOR
	/* initalize colors to global defaults */
	wp->w_fcolor = gfcolor;
	wp->w_bcolor = gbcolor;
#endif
	wp->w_ntrows = term.t_nrow - 1;	/* "-1" for mode line.  */
	wp->w_force = 0;
	wp->w_flag = WFMODE | WFHARD;	/* Full.                */
}


/*****		Compiler specific Library functions	****/

#if RAMSIZE 

/* These routines will allow me to track memory usage by placing a layer on
   top of the standard system malloc() and free() calls.  with this code
   defined, the environment variable, $RAM, will report on the number of
   bytes allocated via malloc.

   with SHOWRAM defined, the number is also posted on the end of the bottom
   mode line and is updated whenever it is changed.
*/

#if	RAMSHOW
 static void dspram( void) ;
#endif

void *allocate( size_t nbytes) {
	nbytes += sizeof nbytes ;			/* add overhead to track allocation */
	size_t *mp = (malloc)( nbytes) ;	/* call the function not the macro */
	if( mp) {
		*mp++ = nbytes ;
		envram += nbytes ;
#if	RAMSHOW
		dspram() ;
#endif
	}

	return mp ;
}

void release( void *mp) {
	if( mp) {
		size_t *sp = mp ;
		sp-- ;
		/* update amount of ram currently malloced */
		envram -= *sp ;
		(free)( sp) ;				/* call the function not the macro */
#if	RAMSHOW
		dspram() ;
#endif
	}
}

#if	RAMSHOW
static void dspram( void) {	/* display the amount of RAM currently malloced */
	char mbuf[ 20] ;

	TTmove( term.t_nrow, term.t_ncol - 12) ;
#if	COLOR
	TTforg( 7) ;
	TTbacg(0) ;
#endif
	sprintf( mbuf, "[%10u]", envram) ;
	char *sp = mbuf ;
	while( *sp)
		TTputc( *sp++) ;

	TTmove( term.t_nrow, 0) ;
	movecursor( term.t_nrow, 0) ;
}
#endif
#endif

/*	On some primitive operation systems, and when emacs is used as
	a subprogram to a larger project, emacs needs to de-alloc its
	own used memory
*/

#if	CLEAN

/* cexit()
 *
 * int status;		return status of emacs
 */
void cexit( int status) {
/* first clean up the windows */
	window_p wp = wheadp ;
	while( wp) {
		window_p tp = wp->w_wndp ;
		free( wp) ;
		wp = tp ;
	}
	
	wheadp = NULL ;

/* then the buffers */
	buffer_p bp ;
	while( (bp = bheadp) != NULL) {
		bp->b_nwnd = 0 ;
		bp->b_flag = 0 ;	/* don't say anything about a changed buffer! */
		zotbuf( bp) ;
	}

/* and the kill buffer */
	kdelete() ;

/* and the video buffers */
	vtfree() ;

	(exit)( status) ;	/* call the function, not the macro */
}
#endif

/* end of main.c */
