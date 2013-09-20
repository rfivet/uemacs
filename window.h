#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "defines.h"	/* COLOR, SCROLLCODE */
#include "buffer.h"		/* buffer, line */

/*
 * There is a window structure allocated for every active display window. The
 * windows are kept in a big list, in top to bottom screen order, with the
 * listhead at "wheadp". Each window contains its own values of dot and mark.
 * The flag field contains some bits that are set by commands to guide
 * redisplay. Although this is a bit of a compromise in terms of decoupling,
 * the full blown redisplay is just too expensive to run for every input
 * character.
 */
struct window {
	struct window *w_wndp;	/* Next window                  */
	struct buffer *w_bufp;	/* Buffer displayed in window   */
	struct line *w_linep;	/* Top line in the window       */
	struct line *w_dotp;	/* Line containing "."          */
	struct line *w_markp;	/* Line containing "mark"       */
	int w_doto;		/* Byte offset for "."          */
	int w_marko;		/* Byte offset for "mark"       */
	char w_toprow;		/* Origin 0 top row of window   */
	char w_ntrows;		/* # of rows of text in window  */
	char w_force;		/* If NZ, forcing row.          */
	char w_flag;		/* Flags.                       */
#if	COLOR
	char w_fcolor;		/* current forground color      */
	char w_bcolor;		/* current background color     */
#endif
};

#define WFFORCE 0x01		/* Window needs forced reframe  */
#define WFMOVE  0x02		/* Movement from line to line   */
#define WFEDIT  0x04		/* Editing within a line        */
#define WFHARD  0x08		/* Better to a full display     */
#define WFMODE  0x10		/* Update mode line.            */
#define	WFCOLR	0x20		/* Needs a color change         */

#if SCROLLCODE
#define WFKILLS 0x40		/* something was deleted        */
#define WFINS   0x80		/* something was inserted       */
#endif

int reposition( int f, int n);
int redraw( int f, int n) ;
int nextwind( int f, int n) ;
int prevwind( int f, int n) ;
int mvdnwind( int f, int n) ;
int mvupwind( int f, int n) ;
int onlywind( int f, int n) ;
int delwind( int f, int n) ;
int splitwind( int f, int n) ;
int enlargewind( int f, int n) ;
int shrinkwind( int f, int n) ;
int resize( int f, int n) ;
int scrnextup( int f, int n) ;
int scrnextdw( int f, int n) ;
int savewnd( int f, int n) ;
int restwnd( int f, int n) ;
int newsize( int f, int n) ;
int newwidth( int f, int n) ;
int getwpos( void) ;
void cknewwindow( void) ;
struct window *wpopup( void) ;  /* Pop up window creation. */

#endif
