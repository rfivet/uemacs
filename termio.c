/* termio.c -- implements termio.h */
#include "termio.h"

#include "defines.h"	/* POSIX */
#ifndef POSIX

/* The functions in this file negotiate with the operating system for
   characters, and write characters in a barely buffered fashion on the
   display.  All operating systems.

   modified by Petri Kutvonen
 */

#include <stdio.h>
#include <stdlib.h>

#include "retcode.h"
#include "utf8.h"


#include <unistd.h>
#include <sys/ioctl.h>

int ttrow = -1 ;    /* Row location of HW cursor */
int ttcol = -1 ;	/* Column location of HW cursor */


#if USG         /* System V */
#include    <signal.h>
#include    <termio.h>
#include    <fcntl.h>
static int kbdflgs ;	/* saved keyboard fd flags      */
static int kbdpoll ;	/* in O_NDELAY mode				*/
static int kbdqp ;		/* there is a char in kbdq      */
static char kbdq ;		/* char we've already read      */
static struct termio otermio ;	/* original terminal characteristics	*/
static struct termio ntermio ;	/* characteristics to use inside		*/
#if XONXOFF
#define XXMASK  0016000
#endif
#endif

#if BSD
#include        <sgtty.h>   /* for stty/gtty functions */
#include    <signal.h>
struct sgttyb ostate;       /* saved tty state */
struct sgttyb nstate;       /* values for editor mode */
struct tchars otchars;      /* Saved terminal special character set */
#if XONXOFF
struct tchars ntchars = { 0xff, 0xff, 0x11, 0x13, 0xff, 0xff };

                /* A lot of nothing and XON/XOFF */
#else
struct tchars ntchars = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

                /* A lot of nothing */
#endif
#if BSD & PKCODE
struct ltchars oltchars;    /* Saved terminal local special character set */
struct ltchars nltchars = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

                /* A lot of nothing */
#endif

#if BSD
#include <sys/ioctl.h>      /* to get at the typeahead */
extern int rtfrmshell();    /* return from suspended shell */
#define TBUFSIZ 128
char tobuf[TBUFSIZ];        /* terminal output buffer */
#endif
#endif

#if SVR4
extern int rtfrmshell();    /* return from suspended shell */
#define TBUFSIZ 128
char tobuf[TBUFSIZ];        /* terminal output buffer */
#endif

/*
 * This function is called once to set up the terminal device streams.
 * On CPM it is a no-op.
 */
void ttopen(void)
{
#if USG
    ioctl(0, TCGETA, &otermio); /* save old settings */
    ntermio.c_iflag = 0;    /* setup new settings */
#if XONXOFF
    ntermio.c_iflag = otermio.c_iflag & XXMASK; /* save XON/XOFF P.K. */
#endif
    ntermio.c_oflag = 0;
    ntermio.c_cflag = otermio.c_cflag;
    ntermio.c_lflag = 0;
    ntermio.c_line = otermio.c_line;
    ntermio.c_cc[VMIN] = 1;
    ntermio.c_cc[VTIME] = 0;
#if PKCODE
    ioctl(0, TCSETAW, &ntermio);    /* and activate them */
#else
    ioctl(0, TCSETA, &ntermio); /* and activate them */
#endif
    kbdflgs = fcntl(0, F_GETFL, 0);
    kbdpoll = FALSE;
#endif

#if BSD
    gtty(0, &ostate);   /* save old state */
    gtty(0, &nstate);   /* get base of new state */
#if XONXOFF
    nstate.sg_flags |= (CBREAK | TANDEM);
#else
    nstate.sg_flags |= RAW;
#endif
    nstate.sg_flags &= ~(ECHO | CRMOD); /* no echo for now... */
    stty(0, &nstate);   /* set mode */
    ioctl(0, TIOCGETC, &otchars);   /* Save old characters */
    ioctl(0, TIOCSETC, &ntchars);   /* Place new character into K */
#if BSD & PKCODE
    ioctl(0, TIOCGLTC, &oltchars);  /* Save old local characters */
    ioctl(0, TIOCSLTC, &nltchars);  /* New local characters */
#endif
#if BSD
    /* provide a smaller terminal output buffer so that
       the type ahead detection works better (more often) */
    setbuffer(stdout, &tobuf[0], TBUFSIZ);
    signal(SIGTSTP, SIG_DFL);   /* set signals so that we can */
    signal(SIGCONT, rtfrmshell);    /* suspend & restart emacs */
#endif
#endif

#if SVR4
    /* provide a smaller terminal output buffer so that
       the type ahead detection works better (more often) */
    setvbuf(stdout, &tobuf[0], _IOFBF, TBUFSIZ);
    signal(SIGTSTP, SIG_DFL);   /* set signals so that we can */
    signal(SIGCONT, rtfrmshell);    /* suspend & restart emacs */
    TTflush();
#endif

    /* on all screens we are not sure of the initial position
       of the cursor                                        */
    ttrow = ttcol = -1 ;
}

/*
 * This function gets called just before we go back home to the command
 * interpreter.
 * Another no-operation on CPM.
 */
void ttclose(void)
{
#if USG
#if PKCODE
    ioctl(0, TCSETAW, &otermio);    /* restore terminal settings */
#else
    ioctl(0, TCSETA, &otermio); /* restore terminal settings */
#endif
    fcntl(0, F_SETFL, kbdflgs);
#endif

#if BSD
    stty(0, &ostate);
    ioctl(0, TIOCSETC, &otchars);   /* Place old character into K */
#if BSD & PKCODE
    ioctl(0, TIOCSLTC, &oltchars);  /* Place old local character into K */
#endif
#endif
}

/*
 * Write a character to the display.
 */
int ttputc( unicode_t c) {
	char utf8[ 4] ;
	int bytes ;

	bytes = unicode_to_utf8( c, utf8) ;
	fwrite( utf8, 1, bytes, stdout) ;
	return 0 ;
}

/*
 * Flush terminal buffer. Does real work where the terminal output is buffered
 * up. A no-operation on systems where byte at a time terminal I/O is done.
 */
void ttflush( void) {
#if	USG | BSD
/*
 * Add some terminal output success checking, sometimes an orphaned
 * process may be left looping on SunOS 4.1.
 *
 * How to recover here, or is it best just to exit and lose
 * everything?
 *
 * jph, 8-Oct-1993
 */

#include <errno.h>

    int status;

    status = fflush(stdout);

    if (status != 0 && errno != EAGAIN) {
        exit(errno);
    }
#endif
}

/*
 * Read a character from the terminal, performing no editing and doing no echo
 * at all.
 * Very simple on CPM, because the system can do exactly what you want.
 */
int ttgetc( void) {
#if BSD
    return 255 & fgetc(stdin);  /* 8BIT P.K. */
#endif

#if USG
    if (kbdqp)
        kbdqp = FALSE;
    else {
        if (kbdpoll && fcntl(0, F_SETFL, kbdflgs) < 0)
            return FALSE;
        kbdpoll = FALSE;
        while (read(0, &kbdq, 1) != 1);
    }
    return kbdq & 255;
#endif
    return 0;
}

#if TYPEAH
/* typahead:    Check to see if any characters are already in the
        keyboard buffer
*/

int typahead( void)
{
#if BSD
    int x;          /* holds # of pending chars */

    return (ioctl(0, FIONREAD, &x) < 0) ? 0 : x;
#endif

#if USG
    if (!kbdqp) {
        if (!kbdpoll && fcntl(0, F_SETFL, kbdflgs | O_NDELAY) < 0)
            return FALSE;
#if PKCODE
        kbdpoll = 1;
#endif
        kbdqp = (1 == read(0, &kbdq, 1));
    }
    return kbdqp;
#endif

#  if !UNIX
    return FALSE;
#  endif
}
# endif

#endif              /* not POSIX */

/* end of termio.c */
