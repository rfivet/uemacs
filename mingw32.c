/* mingw32.c -- */

#ifdef MINGW32
#include "termio.h"
#include "terminal.h"

#include <errno.h>
#include <io.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "utf8.h"
#include "wscreen.h"

static void vv( void) {}
static void vi( int i) {}
static int is( char *s) { return *s ; }

static void ttmove( int l, int c) ;

#define	MARGIN	8
#define	SCRSIZ	64
#define	NPAUSE	10    /* # times thru update to pause. */

struct terminal term = {
	24, /* These four values are set dynamically at open time. */
	24,
	80,
	80,
	MARGIN,
	SCRSIZ,
	NPAUSE,
	ttopen,
#if	PKCODE
	ttclose,
#else
	ttclose,
#endif
	vv,		/* ttkopen, */
	vv,		/* ttkclose, */
	ttgetc,
	ttputc,
	ttflush,
	ttmove,
	vv, 	/* tteeol, */
	vv, 	/* tteeop, */
	vv, 	/* ttbeep, */
	vi, 	/* ttrev, */
	is  	/* ttcres */
#if	COLOR
  , iv, 	/* ttfcol, */
	iv		/* ttbcol */
#endif
#if     SCROLLCODE
  , NULL		/* set dynamically at open time */
#endif
} ;


int ttrow ;		/* Row location of HW cursor */
int ttcol ;		/* Column location of HW cursor */

boolean eolexist = TRUE ;	/* does clear to EOL exist?     */
boolean revexist = FALSE ;	/* does reverse video exist?    */
boolean sgarbf = TRUE ;		/* State of screen unknown      */

char sres[ 16] ;	        /* Current screen resolution.   */
							/* NORMAL, CGA, EGA, VGA	*/

void ttopen( void) {
	winit() ;
	wcls() ;
	term.t_mrow = term.t_nrow = wbottom() - wtop() ;
	term.t_mcol = term.t_ncol = wright() - wleft() + 1 ;
	wtitle( "uEMACS") ;
}

void ttclose( void) {
}

int ttputc( int c) {
	char utf8[ 6] ;
	int bytes ;

	bytes = unicode_to_utf8( c, utf8) ;
	fwrite( utf8, 1, bytes, stdout);
	return 0 ;
}

void ttflush( void) {
	int status ;

	status = fflush( stdout);
	while( status < 0 && errno == EAGAIN) {
		_sleep( 1) ;
		status = fflush( stdout) ;
	}

	if( status < 0)
		exit( 15) ;
}

int ttgetc( void) {
	static char buffer[ 32] ;
	static int pending ;
	unicode_t c ;
	int count, bytes = 1, expected ;

	count = pending ;
	if( !count) {
		count = read( 0, buffer, sizeof( buffer)) ;
		if( count <= 0)
			return 0 ;

		pending = count ;
	}

	c = (unsigned char) buffer[ 0] ;
	if( c >= 32 && c < 128)
		goto done ;

	/*
	 * Lazy. We don't bother calculating the exact
	 * expected length. We want at least two characters
	 * for the special character case (ESC+[) and for
	 * the normal short UTF8 sequence that starts with
	 * the 110xxxxx pattern.
	 *
	 * But if we have any of the other patterns, just
	 * try to get more characters. At worst, that will
	 * just result in a barely perceptible 0.1 second
	 * delay for some *very* unusual utf8 character
	 * input.
	 */
	expected = 2 ;
	if( (c & 0xe0) == 0xe0)
		expected = 6 ;

	/* Special character - try to fill buffer */
	if( count < expected) {
		int n;
#if 0
		ntermios.c_cc[VMIN] = 0;
		ntermios.c_cc[VTIME] = 1;		/* A .1 second lag */
		tcsetattr(0, TCSANOW, &ntermios);
#endif
		n = read(0, buffer + count, sizeof(buffer) - count);

		/* Undo timeout */
#if 0
		ntermios.c_cc[VMIN] = 1;
		ntermios.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &ntermios);
#endif
		if (n > 0)
			pending += n;
	}

	if( pending > 1) {
		unsigned char second = buffer[1];

		/* Turn ESC+'[' into CSI */
		if (c == 27 && second == '[') {
			bytes = 2;
			c = 128+27;
			goto done;
		}
	}

	bytes = utf8_to_unicode( buffer, 0, pending, &c) ;

done:
	pending -= bytes ;
	memmove( buffer, buffer+bytes, pending) ;
	return c ;
}

int typahead( void) {
	int x ;			/* holds # of pending chars */

#ifdef FIONREAD
	if( ioctl( 0, FIONREAD, &x) < 0)
#endif
	x = 0 ;
	return x ;
}

static void ttmove( int l, int c) {
	wgoxy( c, l) ;
}

#else
typedef void _pedantic_empty_translation_unit ;
#endif

/* end of mingw32.c */
