#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "estruct.h"

extern int mpresf ;		/* Stuff in message line */
extern int scrollcount ;	/* number of lines to scroll */
extern int discmd ;		/* display command flag         */
extern int gfcolor ;		/* global forgrnd color (white) */
extern int gbcolor ;		/* global backgrnd color (black) */

void vtinit( void) ;
void vtfree( void) ;
void vttidy( void) ;
void vtmove( int row, int col) ;
int upscreen( int f, int n) ;
int update( int force) ;
void updpos( void) ;
void upddex( void) ;
void updgar( void) ;
int updupd( int force) ;
void upmode( void) ;
void movecursor( int row, int col) ;
void mlerase( void) ;
void mlwrite( const char *fmt, ...) ;
void mlforce( char *s) ;
void mlputs( char *s) ;
void getscreensize( int *widthp, int *heightp) ;

#if UNIX
#include <signal.h>
#ifdef SIGWINCH
extern int chg_width, chg_height ;

void sizesignal( int signr) ;
#endif
#endif

#endif
