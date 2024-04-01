/* display.c -- implements display.h */
#include "display.h"

/* The functions in this file handle redisplay.  There are two halves, the
   ones that update the virtual display screen, and the ones that make the
   physical display screen the same as the virtual display screen.  These
   functions use hints that are left in the windows by the commands.

   Modified by Petri Kutvonen
 */

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "defines.h"
#include "input.h"
#include "line.h"
#include "termio.h"
#include "terminal.h"
#include "version.h"
#include "utf8.h"
#include "window.h"

typedef struct {
	int v_flag ;			/* Flags */
#if	COLOR
	int v_fcolor ;			/* current foreground color */
	int v_bcolor ;			/* current background color */
	int v_rfcolor ;			/* requested foreground color */
	int v_rbcolor ;			/* requested background color */
#endif
	unicode_t v_text[] ;	/* Screen data. */
} *video_p ;

#define VFCHG   0x0001		/* Changed flag                 */
#define	VFEXT	0x0002		/* extended (beyond column 80)  */
#define	VFREV	0x0004		/* reverse video status         */
#define	VFREQ	0x0008		/* reverse video request        */
#define	VFCOL	0x0010		/* color change requested       */

static video_p *vscreen ;	/* Virtual screen. */
static video_p *pscreen ;	/* Physical screen. */

#ifdef SIGWINCH
# include <sys/ioctl.h>
/* for window size changes */
  int chg_width, chg_height ;
  static int displaying = TRUE ;

  static void sizesignal( int signr) ;
#endif


static int currow ;		/* Cursor row                   */
static int curcol ;		/* Cursor column                */
static int vtrow = 0 ;	/* Row location of SW cursor */
static int vtcol = 0 ;	/* Column location of SW cursor */
static int lbound = 0 ;	/* leftmost column of current line being displayed */

int mpresf = FALSE ;	/* TRUE if message in last line */
int scrollcount = 1 ;	/* number of lines to scroll */
int discmd = TRUE ;		/* display command flag         */
int disinp = TRUE ;		/* display input characters (echo)	*/

/* global variables */
boolean viewtab = FALSE ;	/* $viewtab = TRUE to visualize hardcoded tab */

static int reframe( window_p wp) ;
static void updone( window_p wp) ;
static void updall( window_p wp) ;
static int scrolls( int inserts) ;
static void scrscroll( int from, int to, int count) ;
static int texttest( int vrow, int prow) ;
static int endofline( unicode_t *s, int n) ;
static void upddex( void) ;
static void updext( void) ;
static void updgar( void) ;
static void updpos( void) ;
static void updateline( int row) ;
static void updupd( boolean force_f) ;
static void modeline( window_p wp) ;
static void mlputi( int i, int r) ;
static void mlputli( long l, int r) ;
static void mlputf( int s) ;
static void mlputs( const char *s) ;
#if SIGWINCH
static void newscreensize( int h, int w) ;
#endif


/* xmalloc is used at early initialization before memory usage tracking is
   enabled so it bypass the memory tracking macroes.
*/
static void *xmalloc( size_t size) {
	void *ret = (malloc)( size) ;
	if( !ret) {
		fprintf( stderr, "fatal: Out of memory\n") ;
		exit( EXIT_FAILURE) ;
	}

	return ret ;
}

/* Initialize the data structures used by the display code.  The edge
   vectors used to access the screens are set up.  The operating system's
   terminal I/O channel is set up.  All the other things get initialized at
   compile time.  The original window has "WFCHG" set, so that it will get
   completely redrawn on the first call to "update".
 */

static int lastmrow ;		/* remember mrow for later free */

static void vtalloc( int maxrow, int maxcol) {
	lastmrow = maxrow ;		/* remember mrow for later free */
	vscreen = xmalloc( maxrow * sizeof( video_p )) ;
	pscreen = xmalloc( maxrow * sizeof( video_p )) ;
	for( int i = 0 ; i < maxrow ; ++i) {
		video_p vp = xmalloc( sizeof *vp + maxcol * sizeof( unicode_t)) ;
		vp->v_flag = 0 ;
#if	COLOR
		vp->v_rfcolor = 7 ;
		vp->v_rbcolor = 0 ;
#endif
		vscreen[ i] = vp ;
		vp = xmalloc( sizeof *vp + maxcol * sizeof( unicode_t)) ;
		vp->v_flag = 0 ;
		pscreen[ i] = vp ;
	}
}

void updmargin( void) {
#define MINMARGIN 3	/* MINMARGIN - 1 enough for $ + prev before current */
#if MINCOLS < 2 * MINMARGIN + 1
# error	"MINCOLS and MINMARGIN are not consistent"
#endif
	term.t_margin = term.t_ncol / 10 ;
	if( term.t_margin < MINMARGIN)
		term.t_margin = MINMARGIN ;

	term.t_scrsiz = term.t_ncol - 2 * term.t_margin ;
}

void vtinit( void) {
#ifdef SIGWINCH
	signal( SIGWINCH, sizesignal) ;
#endif

	setlocale( LC_CTYPE, "") ; /* expects $LANG like en_GB.UTF-8 */
	TTopen() ;		/* open the screen */
	updmargin() ;
	TTkopen() ;		/* open the keyboard */
	TTrev( FALSE) ;
	vtalloc( term.t_mrow, term.t_mcol) ;
}


/* free up all the dynamically video structures allocated by vtalloc */
void vtfree( void) {
/* as xmalloc bypass the malloc macro, we need bypass the free macro too */
	for( int i = 0 ; i < lastmrow ; ++i ) {
		(free)( vscreen[ i]) ;
		(free)( pscreen[ i]) ;
	}

	(free)( vscreen) ;
	(free)( pscreen) ;
}


/* Clean up the virtual terminal system, in anticipation for a return to
   the operating system.  Move down to the last line and clear it out (the
   next system prompt will be written in the line).  Shut down the channel
   to the terminal.
 */
void vttidy( void) {
	mlerase() ;	/* ends with movecursor( term.t_nrow, 0) and TTflush() */
	TTclose() ;
	TTkclose() ;
#ifdef PKCODE
	int ret = write( 1, "\r", 1) ;
	if( ret != 1) {
		/* some error handling here */
	}
#endif
}


/* Set the virtual cursor to the specified row and column on the virtual
   screen.  There is no checking for nonsense values; this might be a good
   idea during the early stages.
 */
static void vtmove( int row, int col) {
	vtrow = row ;
	vtcol = col ;
}


/* Write a character to the virtual screen.  The virtual row and column are
   updated.  If we are not yet on left edge, don't print it yet.  If the
   line is too long put a "$" in the last column.

   This routine only puts printing characters into the virtual terminal
   buffers.  Only column overflow is checked.
 */

static void sane_vtputc( unicode_t c) {
/* intended to be called by vtputc once sanity check has been done
** only normal printable char should be passed as parameter */
	unicode_t *vcp = vscreen[ vtrow]->v_text ;	/* ptr to line being updated */
	if( vtcol >= term.t_ncol)
		vcp[ term.t_ncol - 1] = '$' ;
	else if( vtcol >= 0)
		vcp[ vtcol] = c ;

	vtcol += 1 ;
}

static void vtputuc( unicode_t c) {
	/* In case somebody passes us a signed char.. */
//	if( c > 0x10FFFF)	/* Let's assume this is due to sign extension */
//		c &= 0xFF ;
	assert( c <= 0x10FFFF) ;

	if( c == '\t') {
		sane_vtputc( viewtab ? 0xBB : ' ') ;	/* 0xBB: '»' */
		while( ((vtcol + lbound) % tabwidth) != 0)
			sane_vtputc( ' ') ;
	} else if( c < 0x20 || c == 0x7F) {
		sane_vtputc( '^') ;
		sane_vtputc( c ^ 0x40) ;
	} else if( c >= 0x80 && c < 0xA0) {
		static const char hex[] = "0123456789abcdef" ;
		sane_vtputc( '\\') ;
		sane_vtputc( hex[ c >> 4]) ;
		sane_vtputc( hex[ c & 15]) ;
	} else if( _utf8_width( c) < 0) {
		sane_vtputc( '\\') ;		/* show as non printable */
		sane_vtputc( 'u') ;
	} else
		sane_vtputc( c) ;
}

static int vtputs( const char *s) {
	int n = 0 ;

	while( *s) {
		unicode_t c ;
		
		s += utf8_to_unicode( s, 0, 4, &c) ;
		vtputuc( c) ;
		n += utf8_width( c) ;
	}

	return n ;
}


/* Erase from the end of the software cursor to the end of the line on which
 * the software cursor is located.
 */
static void vteeol( void) {
	unicode_t *vcp = vscreen[ vtrow]->v_text ;

	while( vtcol < term.t_ncol)
		vcp[ vtcol++] = ' ' ;
}

/* upscreen():
 *	user routine to force a screen update
 *	always finishes complete update
 */
TBINDABLE( upscreen) {
	update( TRUE) ;
	return TRUE ;
}

#if SCROLLCODE
static int scrflags ;
#endif


/* Make sure that the display is right.  This is a three part process.
   First, scan through all of the windows looking for dirty ones.  Check
   the framing, and refresh the screen.  Second, make sure that "currow"
   and "curcol" are correct for the current window.  Third, make the
   virtual and physical screens the same.

   boolean force_f ;		force update past type ahead?
 */
void update( boolean force_f) {
	window_p wp ;

#if	TYPEAH && ! PKCODE
	if( force_f == FALSE && typahead())
		return ;
#endif
#if	VISMAC == 0
	if( force_f == FALSE && kbdmode == PLAY)
		return ;
#endif

#if SIGWINCH
	displaying = TRUE ;
resize:
#endif

#if SCROLLCODE

/* first, propagate mode line changes to all instances of a buffer displayed
 * in more than one window */
	for( wp = wheadp ; wp != NULL ; wp = wp->w_wndp)
		if( wp->w_flag & WFMODE
		&&  wp->w_bufp->b_nwnd > 1)
		/* make sure all previous windows have this */
			for( window_p owp = wheadp ; owp != NULL ; owp = owp->w_wndp)
				if( owp->w_bufp == wp->w_bufp)
					owp->w_flag |= WFMODE ;
#endif

/* update any windows that need refreshing */
	for( wp = wheadp ; wp != NULL ; wp = wp->w_wndp)
		if( wp->w_flag) {
		/* if the window has changed, service it */
			reframe( wp) ;	/* check the framing */
#if SCROLLCODE
			if( wp->w_flag & (WFKILLS | WFINS)) {
				scrflags |= wp->w_flag & (WFINS | WFKILLS) ;
				wp->w_flag &= ~(WFKILLS | WFINS) ;
			}
#endif
			if( (wp->w_flag & ~WFMODE) == WFEDIT)
				updone( wp) ;	/* update EDITed line */
			else if( wp->w_flag & ~WFMOVE)
				updall( wp) ;	/* update all lines */
#if SCROLLCODE
			if( scrflags || (wp->w_flag & WFMODE))
#else
			if( wp->w_flag & WFMODE)
#endif
				modeline( wp) ;	/* update modeline */
			wp->w_flag = 0 ;
			wp->w_force = 0 ;
		}

	/* recalc the current hardware cursor location */
	updpos() ;

	/* check for lines to de-extend */
	upddex() ;

	/* if screen is garbage, re-plot it */
	if( sgarbf != FALSE)
		updgar() ;

	/* update the virtual screen to the physical screen */
	updupd( force_f) ;

	/* update the cursor and flush the buffers */
	movecursor( currow, curcol - lbound) ;
	TTflush() ;
#if SIGWINCH
	if( chg_width || chg_height) {
		newscreensize( chg_height, chg_width) ;
		force_f = TRUE ;
		goto resize ;
	}

	displaying = FALSE ;
#endif
}


/* reframe:
 *	check to see if the cursor is on in the window
 *	and re-frame it if needed or wanted
 */
static int reframe( window_p wp) {
	line_p lp, lp0 ;
	int i = 0 ;

	/* if not a requested reframe, check for a needed one */
	if( (wp->w_flag & WFFORCE) == 0) {
#if SCROLLCODE
		/* loop from one line above the window to one line after */
		lp = wp->w_linep ;
		lp0 = lback( lp) ;
		if( lp0 == wp->w_bufp->b_linep)
			i = 0 ;
		else {
			i = -1 ;
			lp = lp0 ;
		}
		for( ; i <= (int) (wp->w_ntrows) ; i++)
#else
		lp = wp->w_linep ;
		for( i = 0 ; i < wp->w_ntrows ; i++)
#endif
		{
			/* if the line is in the window, no reframe */
			if( lp == wp->w_dotp) {
#if SCROLLCODE
				/* if not _quite_ in, we'll reframe gently */
				if( i < 0 || i == wp->w_ntrows) {
					/* if the terminal can't help, then
					   we're simply outside */
					if( term.t_scroll == NULL)
						i = wp->w_force ;
					break ;
				}
#endif
				return TRUE ;
			}

			/* if we are at the end of the file, reframe */
			if( lp == wp->w_bufp->b_linep)
				break ;

			/* on to the next line */
			lp = lforw( lp) ;
		}
	}
#if SCROLLCODE
	if( i == -1) {		/* we're just above the window */
		i = scrollcount ;	/* put dot at first line */
		scrflags |= WFINS ;
	} else if( i == wp->w_ntrows) {	/* we're just below the window */
		i = -scrollcount ;	/* put dot at last line */
		scrflags |= WFKILLS ;
	} else			/* put dot where requested */
#endif
		i = wp->w_force ;	/* (is 0, unless reposition() was called) */

	wp->w_flag |= WFMODE ;

	/* how far back to reframe? */
	if( i > 0) {		/* only one screen worth of lines max */
		if( --i >= wp->w_ntrows)
			i = wp->w_ntrows - 1 ;
	} else if( i < 0) {	/* negative update???? */
		i += wp->w_ntrows ;
		if( i < 0)
			i = 0 ;
	} else
		i = wp->w_ntrows / 2 ;

	/* backup to new line at top of window */
	lp = wp->w_dotp ;
	while( i != 0 && lback( lp) != wp->w_bufp->b_linep) {
		--i ;
		lp = lback( lp) ;
	}

	/* and reset the current line at top of window */
	wp->w_linep = lp ;
	wp->w_flag |= WFHARD ;
	wp->w_flag &= ~WFFORCE ;
	return TRUE ;
}

static void show_line( line_p lp) {
	int i = 0, len = llength( lp) ;

	while( i < len) {
		unicode_t c ;
		i += utf8_to_unicode( lp->l_text, i, len, &c) ;
		vtputuc( c) ;
	}

	vteeol() ;
}


/* updone:
 *	update the current line	to the virtual screen
 *
 * window_p wp;		window to update current line in
 */
static void updone( window_p wp) {
	line_p lp ;

/* search down the line we want */
	int sline = wp->w_toprow ;	/* physical screen line to update */
	for( lp = wp->w_linep ; lp != wp->w_dotp ; lp = lforw( lp))
		++sline ;

/* and update the virtual line */
	vscreen[ sline]->v_flag |= VFCHG ;
	vscreen[ sline]->v_flag &= ~VFREQ ;
	vtmove( sline, 0) ;
	show_line( lp) ;
#if	COLOR
	vscreen[ sline]->v_rfcolor = wp->w_fcolor ;
	vscreen[ sline]->v_rbcolor = wp->w_bcolor ;
#endif
}


/* updall:
 *	update all the lines in a window on the virtual screen
 *
 * window_p wp;		window to update lines in
 */
static void updall( window_p wp) {
/* search down the lines, updating them */
	line_p lp = wp->w_linep ;	/* line to update */
	int sline = wp->w_toprow ;	/* physical screen line to update */
	while( sline < wp->w_toprow + wp->w_ntrows) {
	/* and update the virtual line */
		vscreen[ sline]->v_flag |= VFCHG ;
		vscreen[ sline]->v_flag &= ~VFREQ ;
		vtmove( sline, 0) ;
		if( lp != wp->w_bufp->b_linep) {
		/* if we are not at the end */
			show_line( lp) ;
			lp = lforw( lp) ;
		} else
			vteeol() ;

	/* on to the next one */
#if	COLOR
		vscreen[ sline]->v_rfcolor = wp->w_fcolor ;
		vscreen[ sline]->v_rbcolor = wp->w_bcolor ;
#endif
		++sline ;
	}
}


/* updpos:
    update the position of the hardware cursor and handle extended lines.
    This is the only update for simple moves.
 */
static void updpos( void) {
	line_p lp ;

/* find the current row */
	currow = curwp->w_toprow ;
	for( lp = curwp->w_linep ; lp != curwp->w_dotp ; lp = lforw( lp))
		++currow ;

/* find the current column */
	curcol = 0 ;
	int i = 0 ;
	while( i < curwp->w_doto) {
		unicode_t c ;

		i += utf8_to_unicode( lp->l_text, i, curwp->w_doto, &c) ;
		if( c == '\t')
			curcol += tabwidth - curcol % tabwidth ;
		else if( c < 0x20 || c == 0x7F)
			curcol += 2 ;	/* displayed as ^c */
		else if( c >= 0x80 && c < 0xA0)
			curcol += 3 ;	/* displayed as \xx */
		else
			curcol += utf8_width( c) ;	/* non printable are displayed as \u */
	}

/* if extended, flag so and update the virtual line image */
	if( curcol >= term.t_ncol - 1) {
		vscreen[ currow]->v_flag |= (VFEXT | VFCHG) ;
		updext() ;
	} else
		lbound = 0 ;
}


/* upddex:
 *	de-extend any line that deserves it
 */
static void upddex( void) {
	for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
		line_p lp = wp->w_linep ;
		for( int i = wp->w_toprow ; i < wp->w_toprow + wp->w_ntrows ; i++) {
			if( vscreen[ i]->v_flag & VFEXT) {
				if( (wp != curwp)
				||	(lp != wp->w_dotp)
				||  (curcol < term.t_ncol - 1)) {
					vtmove( i, 0) ;
					show_line( lp) ;

				/* this line no longer is extended */
					vscreen[ i]->v_flag &= ~VFEXT ;
					vscreen[ i]->v_flag |= VFCHG ;
				}
			}

			lp = lforw( lp) ;
		}
	}
}


/* updgar:
 *	if the screen is garbage, clear the physical screen and
 *	the virtual screen and force a full update
 */
static void updgar( void) {
	for( int i = 0 ; i < term.t_nrow ; ++i) {
		vscreen[ i]->v_flag |= VFCHG ;
		vscreen[ i]->v_flag &= ~VFREV ;
#if	COLOR
		vscreen[ i]->v_fcolor = gfcolor ;
		vscreen[ i]->v_bcolor = gbcolor ;
#endif
		unicode_t *txt = pscreen[ i]->v_text ;
		for( int j = 0 ; j < term.t_ncol ; ++j)
			txt[ j] = ' ' ;
	}

	movecursor( 0, 0) ;	/* Erase the screen. */
	term.t_eeop() ;
	sgarbf = FALSE ;	/* Erase-page clears */
	mpresf = FALSE ;	/* the message area. */
#if	COLOR
	mlerase() ;			/* needs to be cleared if colored */
#endif
}


/* updupd:
 *	update the physical screen from the virtual screen
 *
 * int force;		forced update flag
 */
static void updupd( boolean force_f) {
#if SCROLLCODE
	if( scrflags & WFKILLS)
		scrolls( FALSE) ;
		
	if( scrflags & WFINS)
		scrolls( TRUE) ;

	scrflags = 0 ;
#endif

	for( int i = 0 ; i < term.t_nrow ; ++i) {
	/* for each line that needs to be updated */
		if( (vscreen[ i]->v_flag & VFCHG) != 0) {
#if	TYPEAH && ! PKCODE
			if( force_f == FALSE && typahead())
				return TRUE ;
#endif
			updateline( i) ;
		}
	}
}

#if SCROLLCODE

/* optimize out scrolls (line breaks, and newlines)
 * arg. chooses between looking for inserts or deletes

 * returns true if it does something
 */
static int scrolls( int inserts) {
	int i, j, k ;
	int count, target, end ;
	int to ;

	if( !term.t_scroll)	/* no way to scroll */
		return FALSE ;

	int rows = term.t_nrow ;
	int cols = term.t_ncol ;

	for( i = 0 ; i < rows ; i++)	/* find first wrong line */
		if( !texttest( i, i))
			break ;

	if( i == rows)		/* no text changes */
		return FALSE ;

	int first = i ;

	video_p vpv = vscreen[ first] ;
	video_p vpp = pscreen[ first] ;

	if( inserts) {
	/* determine types of potential scrolls */
		end = endofline( vpv->v_text, cols) ;
		if( end == 0)
			target = first ;	/* newlines */
		else if( memcmp( vpp->v_text, vpv->v_text, 4*end) == 0)
			target = first + 1 ;	/* broken line newlines */
		else
			target = first ;
	} else {
		target = first + 1 ;
	}

	/* find the matching shifted area */
	int match = -1 ;
	int longmatch = -1 ;
	int longcount = 0 ;
	int from = target ;
	for( i = from + 1 ; i < rows - longcount /* P.K. */ ; i++) {
		if( inserts ? texttest( i, from) : texttest( from, i)) {
			match = i ;
			count = 1 ;
			for( j = match + 1, k = from + 1 ;
			     j < rows && k < rows ; j++, k++) {
				if( inserts ? texttest( j, k) :
				    texttest( k, j))
					count++ ;
				else
					break ;
			}
			if( longcount < count) {
				longcount = count ;
				longmatch = match ;
			}
		}
	}

	match = longmatch ;
	count = longcount ;

	if( !inserts) {
		/* full kill case? */
		if( match > 0 && texttest( first, match - 1)) {
			target-- ;
			match-- ;
			count++ ;
		}
	}

	/* do the scroll */
	if( match > 0 && count > 2) {	/* got a scroll */
		/* move the count lines starting at target to match */
		if( inserts) {
			from = target ;
			to = match ;
		} else {
			from = match ;
			to = target ;
		}
		if( 2 * count < abs( from - to))
			return FALSE ;
		scrscroll( from, to, count) ;
		for( i = 0 ; i < count ; i++) {
			vpp = pscreen[ to + i] ;
			vpv = vscreen[ to + i] ;
			memcpy( vpp->v_text, vpv->v_text, 4*cols) ;
			vpp->v_flag = vpv->v_flag ;	/* XXX */
			if( vpp->v_flag & VFREV) {
				vpp->v_flag &= ~VFREV ;
				vpp->v_flag |= VFREQ ;
			}
		}

		if( inserts) {
			from = target ;
			to = match ;
		} else {
			from = target + count ;
			to = match + count ;
		}

		for( i = from ; i < to ; i++) {
			unicode_t *txt ;
			txt = pscreen[ i]->v_text ;
			for( j = 0 ; j < term.t_ncol ; ++j)
				txt[ j] = ' ' ;
			vscreen[ i]->v_flag |= VFCHG ;
		}

		return TRUE ;
	}

	return FALSE ;
}


/* move the "count" lines starting at "from" to "to" */
static void scrscroll( int from, int to, int count) {
	ttrow = ttcol = -1 ;
	term.t_scroll( from, to, count) ;
}


/* return TRUE on text match
 *
 * int vrow, prow;		virtual, physical rows
 */
static int texttest( int vrow, int prow) {
	video_p vpv = vscreen[ vrow] ;	/* virtual screen image */
	video_p vpp = pscreen[ prow] ;	/* physical screen image */

	return !memcmp( vpv->v_text, vpp->v_text, 4*term.t_ncol) ;
}


/* return the index of the first blank of trailing whitespace */
static int endofline( unicode_t *s, int n) {
	int i ;
	for( i = n - 1 ; i >= 0 ; i--)
		if( s[ i] != ' ')
			return i + 1 ;
	return 0 ;
}

#endif				/* SCROLLCODE */


/* updext:
   update the extended line which the cursor is currently on at a column
   greater than the terminal width.  The line will be scrolled right or
   left to let the user see where the cursor is.
 */
static void updext( void) {
	/* calculate what column the real cursor will end up in */
	lbound = curcol - ((curcol - term.t_ncol) % term.t_scrsiz + term.t_margin) ;

	/* scan through the line outputing characters to the virtual screen */
	/* once we reach the left edge                                  */
	vtmove( currow, -lbound) ;	/* start scanning offscreen */
	show_line( curwp->w_dotp) ;

	/* put a '$' in column 1 */
	vscreen[ currow]->v_text[ 0] = '$' ;
}


/* Update a single line. This does not know how to use insert or delete
 * character sequences; we are using VT52 functionality. Update the physical
 * row and column variables. It does try an exploit erase to end of line.
 */

/* updateline()
 *
 * int row ;	row of screen to update
 */
static void updateline( int row) {
	video_p vp1 = vscreen[ row] ;
	vp1->v_flag &= ~VFCHG ;	/* flag this line as updated */

	/* set up pointers to virtual and physical lines */
	unicode_t *inp = vp1->v_text ;
	unicode_t *out = pscreen[ row]->v_text ;

#if	COLOR
	TTforg( vp1->v_rfcolor) ;
	TTbacg( vp1->v_rbcolor) ;
#endif

/* do a re-write of the entire line if there is a request to toggle the
 * reverse status */
	int rev = (vp1->v_flag & VFREV) == VFREV ;
	if( rev != ((vp1->v_flag & VFREQ) == VFREQ)
#if	COLOR
	||	(vp1->v_fcolor != vp1->v_rfcolor)
	||	(vp1->v_bcolor != vp1->v_rbcolor)
#endif
	) {
		movecursor( row, 0) ;	/* Go to start of line. */
		TTrev( !rev) ;		/* set needed rev video state */

		/* scan through the line and dump it to the screen and
		   the virtual screen array                             */
		while( ttcol < term.t_ncol) {
		/* TODO: handle double width unicode char at last screen col */
			unicode_t c = *out++ = *inp++ ;
			TTputc( c) ;
			ttcol += _utf8_width( c) ;	/* filtered by vtputuc */
		}

		TTrev( FALSE) ;		/* turn rev video off */

		/* update the needed flags */
		vp1->v_flag ^= VFREV ;
#if	COLOR
		vp1->v_fcolor = vp1->v_rfcolor ;
		vp1->v_bcolor = vp1->v_rbcolor ;
#endif
		return ;
	}

	/* advance past any common chars at the left */
	unicode_t *end = &vp1->v_text[ term.t_ncol] ;
	int startcol = 0 ;
	while( inp != end && *inp == *out) {
		startcol += _utf8_width( *inp++) ;	/* filtered by vtputuc */
		++out ;
	}

/* This can still happen, even though we only call this routine on changed
 * lines. A hard update is always done when a line splits, a massive
 * change is done, or a buffer is displayed twice. This optimizes out most
 * of the excess updating. A lot of computes are used, but these tend to
 * be hard operations that do a lot of update, so I don't really care.
 */
	/* if both lines are the same, no update needs to be done */
	if( inp == end)
		return ;

	/* find out if there is a match on the right */
	int nbflag = FALSE ;	/* non-blanks to the right flag? */

	unicode_t *nbp = &pscreen[ row]->v_text[ term.t_ncol] ;
	while( end[ -1] == nbp[ -1]) {
		--end ;
		--nbp ;
		if( *end != ' ')	/* Note if any nonblank */
			nbflag = TRUE ;	/* in right match. */
	}

	nbp = end ;

	/* Erase to EOL ? */
	if( nbflag == FALSE && eolexist == TRUE && (rev != TRUE)) {
		while( nbp != inp && nbp[ -1] == ' ')
			--nbp ;

		if( end - nbp <= 3)	/* Use only if erase is */
			nbp = end ;		/* fewer characters. */
	}

	movecursor( row, startcol) ;	/* Go to start of line change */
	TTrev( rev) ;

	while( inp != nbp) {	/* Copy */
		unicode_t c = *out++ = *inp++ ;
		TTputc( c) ;
		ttcol += utf8_width( c) ;
	}

	if( inp != end) {		/* Erase */
		TTeeol() ;
		do
			*out++ = ' ' ;
		while( ++inp != end) ;
	}

	TTrev( FALSE) ;
}


/* Redisplay the mode line for the window pointed to by the "wp".  This is
   the only routine that has any idea of how the modeline is formatted.
   You can change the modeline format by hacking at this routine.  Called
   by "update" any time there is a dirty window.
 */
static void modeline( window_p wp) {
	char tline[] = "   % " ;	/* formatting buffer for percentage */

	int n = wp->w_toprow + wp->w_ntrows ;			/* Location. */
	vscreen[ n]->v_flag |= VFCHG | VFREQ | VFCOL ;	/* Redraw next time. */
#if	COLOR
	vscreen[ n]->v_rfcolor = 0 ;	/* black on */
	vscreen[ n]->v_rbcolor = 7 ;	/* white..... */
#endif
	vtmove( n, 0) ;		/* Seek to right line. */
	int lchar = "-= -"[ 2 * revexist + (wp == curwp)] ;	/* pick bg character */

	buffer_p bp = wp->w_bufp ;
	vtputuc( ((bp->b_flag & BFTRUNC) != 0) ? '#' : lchar) ; /* truncated? */
	vtputuc( ((bp->b_flag & BFCHG) != 0)   ? '*' : lchar) ; /* changed? */
	vtputuc( ' ') ;

	if( n == term.t_nrow - 1)
		n = 3 + vtputs( PROGRAM_NAME_UTF8 " " VERSION ": ") ;
	else
		n = 3 ;

	n += vtputs( bp->b_bname) ;
	n += vtputs( " (") ;

/* display the modes */
	int pos = n ;
	if( (bp->b_flag & BFTRUNC) != 0)
		n += vtputs( "Truncated") ;

	for( int i = 0 ; i < NUMMODES ; i++)	/* add in the mode flags */
		if( wp->w_bufp->b_mode & (1 << i)) {
			if( n > pos) {
				vtputuc( ' ') ;
				n += 1 ;
			}

			n += vtputs( modename[ i]) ;
		}

	n += vtputs( ") ") ;

	if( bp->b_fname[ 0] != 0 && strcmp( bp->b_bname, bp->b_fname) != 0) {
		n += vtputs( bp->b_fname) ;
		vtputuc( ' ') ;
		++n ;
	}

	while( n < term.t_ncol) {	/* Pad to full width. */
		vtputuc( lchar) ;
		++n ;
	}

/* determine if top line, bottom line, or both are visible */
	char *msg = NULL ;

	if( wp->w_linep == wp->w_bufp->b_linep)
			msg = "Empty" ;
	else {
		if( lback( wp->w_linep) == wp->w_bufp->b_linep)
			msg = " Top " ;

		line_p lp = wp->w_linep ;
		for( int rows = wp->w_ntrows ; rows > 0 ; rows--) {
			lp = lforw( lp) ;
			if( lp == wp->w_bufp->b_linep) {
				msg = msg ? " All " : " End " ;
				break ;
			}
		}

		if( !msg) {
		/* buffer is not empty, both first and last line not in window */
			n = 0 ;		/* count of lines in buffer */
			pos = 0 ;	/* line number of the top line in window */
			for( lp = lforw( bp->b_linep) ; lp != bp->b_linep ;
															lp = lforw( lp)) {
				n += 1 ;
				if( lp == wp->w_linep)
					pos = n ;
			}

			pos = (100L * pos) / n ;
			tline[ 2] = pos % 10 + '0' ;
			pos /= 10 ;
			if( pos)
				tline[ 1] = pos + '0' ;

			msg = tline ;
		}
	}

	vtcol -= 7 ;	/* strlen(" top ") plus a couple */
	vtputs( msg) ;
}

void upmode( void) {				/* update all the mode lines */
	window_p wp ;

	wp = wheadp ;
	while( wp != NULL) {
		wp->w_flag |= WFMODE ;
		wp = wp->w_wndp ;
	}
}


/* Send a command to the terminal to move the hardware cursor to row "row"
 * and column "col". The row and column arguments are origin 0. Optimize out
 * random calls. Update "ttrow" and "ttcol".
 */
void movecursor( int row, int col) {
	if( row != ttrow || col != ttcol) {
		ttrow = row ;
		ttcol = col ;
		TTmove( row, col) ;
	}
}


/* Erase the message line. This is a special routine because the message line
 * is not considered to be part of the virtual screen. It always works
 * immediately; the terminal buffer is flushed via a call to the flusher.
 */
void mlerase( void) {
	movecursor( term.t_nrow, 0) ;
	if( discmd != FALSE) {
#if	COLOR
		TTforg( 7) ;
		TTbacg( 0) ;
#endif
		if( eolexist == TRUE)
			TTeeol() ;
		else {
			for( ttcol = 0 ; ttcol < term.t_ncol ; ttcol++)
				TTputc( ' ') ;

			movecursor( term.t_nrow, 0) ;
		}

		mpresf = FALSE ;
	}

	TTflush() ;
}

static void mlputc( unicode_t c) {
	if( ttcol < term.t_ncol) {
		TTputc( c) ;
		ttcol += utf8_width( c) ;
	}
}


/* output a string of output characters
 *
 * char *s;     string to output
 */
void ostring( const char *s) {
	if( discmd)
		mlputs( s) ;
}



/* Write a message into the message line. Keep track of the physical cursor
 * position. A small class of printf like format items is handled. Assumes the
 * stack grows down; this assumption is made by the "++" in the argument scan
 * loop. Set the "message line" flag TRUE.
 *
 * char *fmt;		format string for output
 * char *arg;		pointer to first argument to print
 */
void vmlwrite( const char *fmt, va_list ap) {
	/* if we are not currently echoing on the command line, abort this */
	if( discmd == FALSE) {
		movecursor( term.t_nrow, 0) ;
		return ;
	}
#if	COLOR
	/* set up the proper colors for the command line */
	TTforg( 7) ;
	TTbacg( 0) ;
#endif

	/* if we can not erase to end-of-line, do it manually */
	if( eolexist == FALSE)
		mlerase() ;	/* ends with movecursor( term.t_nrow, 0) and TTflush() */
	else
		movecursor( term.t_nrow, 0) ;

	mpresf = *fmt ? TRUE : FALSE ;	/* flag if line has content or not */
	while( *fmt) {
		unicode_t c ;

		fmt += utf8_to_unicode( fmt, 0, 4, &c) ;
		if( c != '%')
			mlputc( c) ;
		else if( *fmt == 0) {
			mlputc( '%') ;
			break ;
		} else {
			fmt += utf8_to_unicode( fmt, 0, 4, &c) ;
			switch( c) {
			case 'd':
				mlputi( va_arg( ap, int), 10) ;
				break ;

			case 'o':
				mlputi( va_arg( ap, int), 8) ;
				break ;

			case 'x':
				mlputi( va_arg( ap, int), 16) ;
				break ;

			case 'D':
				mlputli( va_arg( ap, long), 10) ;
				break ;

			case 's':
				mlputs( (char *) va_arg( ap, char *)) ;
				break ;

			case 'f':
				mlputf( va_arg( ap, int)) ;
				break ;

			case 'B':	/* ring a bell */
				TTbeep() ;
				break ;

			default:
				mlputc( '%') ;
				/* fallthrough */
			case '%':
				mlputc( c) ;
			}
		}
	}

	/* if we can, erase to the end of screen */
	if( eolexist == TRUE && ttcol < term.t_ncol)
		TTeeol() ;

	TTflush() ;
}

void mlwrite( const char *fmt, ...) {
	va_list ap ;
	
	va_start( ap, fmt) ;
	vmlwrite( fmt, ap) ;
	va_end( ap) ;
}


/* Write out a string. Update the physical cursor position. This assumes that
 * the characters in the string all have width "1"; if this is not the case
 * things will get screwed up a little.
 */
static void mlputs( const char *s) {
	while( *s && (ttcol < term.t_ncol)) {
		unicode_t uc ;

		s += utf8_to_unicode( (char *) s, 0, 4, &uc) ;
		if( uc == '\t')	/* Don't render tabulation */
			uc = ' ' ;

		TTputc( uc) ;
		ttcol += utf8_width( uc) ;
	}
}


/* Write out an integer, in the specified radix. Update the physical cursor
 * position.
 */
static void mlputi( int i, int r) {
	int q ;
	unsigned u ;
	static char hexdigits[] = "0123456789ABCDEF" ;

	if( r == 16 || i >= 0)
		u = i ;
	else {
		u = -i ;
		mlputc( '-') ;
	}

	q = u / r ;

	if( q != 0)
		mlputi( q, r) ;

	mlputc( hexdigits[ u % r]) ;
}


/* do the same except as a long integer. */
static void mlputli( long l, int r) {
	long q ;

	if( l < 0) {
		l = -l ;
		mlputc( '-') ;
	}

	q = l / r ;

	if( q != 0)
		mlputli( q, r) ;

	mlputc( (int) (l % r) + '0') ;
}


/* write out a scaled integer with two decimal places
 *
 * int s;		scaled integer to output
 */
static void mlputf( int s) {
	int i ;		/* integer portion of number */
	int f ;		/* fractional portion of number */

	/* break it up */
	i = s / 100 ;
	f = s % 100 ;

	/* send out the integer portion */
	mlputi( i, 10) ;
	mlputc( '.') ;
	mlputc( (f / 10) + '0') ;
	mlputc( (f % 10) + '0') ;
}


/* Get terminal size from system.
   Store number of lines into *heightp and width into *widthp.
   If zero or a negative number is stored, the value is not valid.  */

void getscreensize( int *widthp, int *heightp) {
#ifdef TIOCGWINSZ
	struct winsize size ;
	*widthp = 0 ;
	*heightp = 0 ;
	if( ioctl( 0, TIOCGWINSZ, &size) < 0)
		return ;
	*widthp = size.ws_col ;
	*heightp = size.ws_row ;
#else
	*widthp = 0 ;
	*heightp = 0 ;
#endif
}

#ifdef SIGWINCH
static void sizesignal( int signr) {
	int w, h ;
	int old_errno = errno ;

	getscreensize( &w, &h) ;

	if( h > 0 && w > 0) {
		term.t_mrow = h = h < term.t_maxrow ? h : term.t_maxrow ;
		term.t_mcol = w = w < term.t_maxcol ? w : term.t_maxcol ;
		if( h - 1 != term.t_nrow || w != term.t_ncol) {
			if( displaying) {
				chg_width = w ;
				chg_height = h ;
			} else {
				newscreensize( h, w) ;
				update( TRUE) ;
			}
		}
	}

	signal( SIGWINCH, sizesignal) ;
	errno = old_errno ;
}

static void newscreensize( int h, int w) {
	chg_width = chg_height = 0 ;
	vtfree() ;
	if( h < MINROWS)
		h = MINROWS ;

	if( w < MINCOLS)
		w = MINCOLS ;

	vtalloc( h, w) ;
	if( h <= term.t_mrow)
		newsize( TRUE, h) ;

	if( w <= term.t_mcol)
		newwidth( TRUE, w) ;
}
#endif


/* output a character when echo is enabled
 *
 * char c ;     character to output
 */
void echoc( unicode_t c) {
	if( disinp) {
		TTputc( c) ;
		ttcol += 1 ;
	}
}


/* output a string of characters when display input is enabled
 *
 * char *s;     string to output
 */
void echos( const char *s) {
	unicode_t c ;

	if( disinp)
		while( (c = *s++)) {
			TTputc( c) ;
			ttcol += 1 ;
		}
}


void rubout( void) {
	if( disinp) {
		TTputc( '\b') ;
		TTputc( ' ') ;
		TTputc( '\b') ;
		ttcol -= 1 ;
	}
}

/* end of display.c */
