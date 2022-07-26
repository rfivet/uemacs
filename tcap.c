/* tcap.c -- implements terminal.h */
#include "terminal.h"

/*	Unix V7 SysV and BS4 Termcap video driver
 *
 *	modified by Petri Kutvonen
 */

#if TERMCAP

#include <stdlib.h>
#include <string.h>

/*
 * Defining this to 1 breaks tcapopen() - it doesn't check if the
 * sceen size has changed.
 *	-lbt
 */
#define USE_BROKEN_OPTIMIZATION 0
#define	termdef	1 /* Don't define "term" external. */

#include <curses.h>
#include <term.h>

#include "defines.h"
#include "display.h"
#include "termio.h"

boolean eolexist = TRUE ;	/* does clear to EOL exist      */
boolean revexist = FALSE ;	/* does reverse video exist?    */
boolean sgarbf = TRUE ;		/* TRUE if screen is garbage    */

char sres[ 16] ;		/* current screen resolution    */
				/* NORMAL, CGA, EGA, VGA	*/

# if UNIX
# include <signal.h>
# endif

#define	MARGIN	8
#define	SCRSIZ	64
#define	NPAUSE	10    /* # times thru update to pause. */
#define BEL     0x07
#define ESC     0x1B

static void tcapkopen(void);
static void tcapkclose(void);
static void tcapmove(int, int);
static void tcapeeol(void);
static void tcapeeop(void);
static void tcapbeep(void);
static void tcaprev(int);
static int tcapcres(char *);
static void tcapscrollregion(int top, int bot);
static void putpad(char *str);

static void tcapopen(void);
# if PKCODE
   static void tcapclose(void);
# endif

# if COLOR
   static void tcapfcol(void);
   static void tcapbcol(void);
# endif
# if SCROLLCODE
   static void tcapscroll_reg(int from, int to, int linestoscroll);
   static void tcapscroll_delins(int from, int to, int linestoscroll);
# endif

#define TCAPSLEN 315
static char tcapbuf[TCAPSLEN];
static char *_UP, _PC, *CM, *CE, *CL, *SO, *SE;

# if PKCODE
   static char *TI, *TE;
#  if USE_BROKEN_OPTIMIZATION
    static int term_init_ok = 0;
#  endif
# endif

# if SCROLLCODE
   static char *CS, *DL, *AL, *SF, *SR;
# endif

terminal_t term = {
	480,	/* actual 479 on 2560x1440 landscape terminal window */
	2550,	/* actual 2541 */
	0,		/* These four values are set dynamically at open time. */
	0,
	0,
	0,
	MARGIN,
	SCRSIZ,
	NPAUSE,
	tcapopen,
# if PKCODE
	tcapclose,
# else
	ttclose,
# endif
	tcapkopen,
	tcapkclose,
	ttgetc,
	ttputc,
	ttflush,
	tcapmove,
	tcapeeol,
	tcapeeop,
	tcapbeep,
	tcaprev,
	tcapcres
# if COLOR
    , tcapfcol,
	tcapbcol
# endif
# if SCROLLCODE
    , NULL		/* set dynamically at open time */
# endif
} ;

static void tcapopen(void)
{
	char *t, *p;
	char tcbuf[1024];
	char *tv_stype;
	int int_col, int_row;

# if PKCODE && USE_BROKEN_OPTIMIZATION
	if (!term_init_ok) {
# endif
		if ((tv_stype = getenv("TERM")) == NULL) {
			fputs( "Environment variable TERM not defined!\n", stderr) ;
			exit( EXIT_FAILURE) ;
		}

		if ((tgetent(tcbuf, tv_stype)) != 1) {
			fputs( "Unknown terminal type ", stderr) ;
			fputs( tv_stype, stderr) ;
			fputs( "!\n", stderr) ;
			exit( EXIT_FAILURE) ;
		}

		/* Get screen size from system, or else from termcap.  */
		getscreensize( &int_col, &int_row) ;
		if( (int_row <= 0)
		    && ((int_row = tgetnum("li")) == -1)) {
			fputs( "termcap entry incomplete (lines)\n", stderr) ;
			exit( EXIT_FAILURE) ;
		}

		if( (int_col <= 0)
		    && ((int_col = tgetnum("co")) == -1)) {
			fputs( "Termcap entry incomplete (columns)\n", stderr) ;
			exit( EXIT_FAILURE) ;
		}

		term.t_mrow = int_row < term.t_maxrow ? int_row : term.t_maxrow ;
		term.t_nrow = term.t_mrow - 1 ;
		term.t_mcol = int_col < term.t_maxcol ? int_col : term.t_maxcol ;
		term.t_ncol = term.t_mcol ;

		p = tcapbuf;
		t = tgetstr("pc", &p);
		if (t)
			_PC = *t;
		else
			_PC = 0;

		CL = tgetstr("cl", &p);
		CM = tgetstr("cm", &p);
		CE = tgetstr("ce", &p);
		_UP = tgetstr("up", &p);
		SE = tgetstr("se", &p);
		SO = tgetstr("so", &p);
		if (SO != NULL)
			revexist = TRUE;
# if PKCODE
		if (tgetnum("sg") > 0) {	/* can reverse be used? P.K. */
			revexist = FALSE;
			SE = NULL;
			SO = NULL;
		}
		TI = tgetstr("ti", &p);	/* terminal init and exit */
		TE = tgetstr("te", &p);
# endif

		if (CL == NULL || CM == NULL || _UP == NULL) {
			fputs( "Incomplete termcap entry\n", stderr) ;
			exit( EXIT_FAILURE) ;
		}

		if (CE == NULL)	/* will we be able to use clear to EOL? */
			eolexist = FALSE;
# if SCROLLCODE
		CS = tgetstr("cs", &p);
		SF = tgetstr("sf", &p);
		SR = tgetstr("sr", &p);
		DL = tgetstr("dl", &p);
		AL = tgetstr("al", &p);

		if (CS && SR) {
			if (SF == NULL)	/* assume '\n' scrolls forward */
				SF = "\n";
			term.t_scroll = tcapscroll_reg;
		} else if (DL && AL) {
			term.t_scroll = tcapscroll_delins;
		} else {
			term.t_scroll = NULL;
		}
# endif

		if (p >= &tcapbuf[TCAPSLEN]) {
			fputs( "Terminal description too big!\n", stderr) ;
			exit( EXIT_FAILURE) ;
		}
# if PKCODE && USE_BROKEN_OPTIMIZATION
		term_init_ok = 1;
	}
# endif
	ttopen();
}

# if PKCODE
static void tcapclose(void)
{
	putpad(tgoto(CM, 0, term.t_nrow));
	putpad(TE);
	ttflush();
	ttclose();
}
# endif

static void tcapkopen( void) {
# if PKCODE
	putpad( TI) ;
	ttflush() ;
	ttrow = ttcol = -1 ;
	sgarbf = TRUE ;
# endif
	strcpy(sres, "NORMAL") ;
}

static void tcapkclose(void)
{
# if PKCODE
	putpad(TE);
	ttflush();
# endif
}

static void tcapmove(int row, int col)
{
	putpad(tgoto(CM, col, row));
}

static void tcapeeol(void)
{
	putpad(CE);
}

static void tcapeeop(void)
{
	putpad(CL);
}

/*
 * Change reverse video status
 *
 * @state: FALSE = normal video, TRUE = reverse video.
 */
static void tcaprev(int state)
{
	if (state) {
		if (SO != NULL)
			putpad(SO);
	} else if (SE != NULL)
		putpad(SE);
}

/* Change screen resolution. */
static int tcapcres(char *res)
{
	return TRUE;
}

# if SCROLLCODE

/* move howmanylines lines starting at from to to */
static void tcapscroll_reg(int from, int to, int howmanylines)
{
	int i;
	if (to == from)
		return;
	if (to < from) {
		tcapscrollregion(to, from + howmanylines - 1);
		tcapmove(from + howmanylines - 1, 0);
		for (i = from - to; i > 0; i--)
			putpad(SF);
	} else {		/* from < to */
		tcapscrollregion(from, to + howmanylines - 1);
		tcapmove(from, 0);
		for (i = to - from; i > 0; i--)
			putpad(SR);
	}
	tcapscrollregion(0, term.t_nrow);
}

/* move howmanylines lines starting at from to to */
static void tcapscroll_delins(int from, int to, int howmanylines)
{
	int i;
	if (to == from)
		return;
	if (to < from) {
		tcapmove(to, 0);
		for (i = from - to; i > 0; i--)
			putpad(DL);
		tcapmove(to + howmanylines, 0);
		for (i = from - to; i > 0; i--)
			putpad(AL);
	} else {
		tcapmove(from + howmanylines, 0);
		for (i = to - from; i > 0; i--)
			putpad(DL);
		tcapmove(from, 0);
		for (i = to - from; i > 0; i--)
			putpad(AL);
	}
}

/* cs is set up just like cm, so we use tgoto... */
static void tcapscrollregion(int top, int bot)
{
	ttputc(_PC);
	putpad(tgoto(CS, bot, top));
}

# endif

# if COLOR
/* No colors here, ignore this. */
static void tcapfcol(void)
{
}
/* No colors here, ignore this. */
static void tcapbcol(void)
{
}
# endif

static void tcapbeep(void)
{
	ttputc(BEL);
}

static void putpad(char *str)
{
	tputs( str, 1, (int (*)( int)) ttputc) ;
}
#endif /* TERMCAP */

/* end of tcap.c */
