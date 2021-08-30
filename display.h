/* display.h -- display functionality */
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdarg.h>

#include "defines.h"		/* UNIX */
#include "names.h"          /* BINDABLE() */
#include "utf8.h"           /* unicode_t */

extern int mpresf ;         /* Stuff in message line */
extern int scrollcount ;    /* number of lines to scroll */
extern int discmd ;         /* display command flag         */
extern int disinp ;         /* display input characters (echo)  */
extern int gfcolor ;        /* global forgrnd color (white) */
extern int gbcolor ;        /* global backgrnd color (black) */

/* global variables */
extern boolean viewtab ;    /* $viewtab = TRUE to visualize hardcoded tab */

/* Bindable functions */
TBINDABLE( upscreen) ;

void vtinit( void) ;
void vtfree( void) ;
void vttidy( void) ;
void update( boolean force_f) ;
void updmargin( void) ;
void upmode( void) ;
void movecursor( int row, int col) ;
void mlerase( void) ;
void vmlwrite( const char *fmt, va_list ap) ;
void mlwrite( const char *fmt, ...) ;
void ostring( const char *s) ;
void echoc( unicode_t c) ;
void echos( const char *s) ;
void rubout( void) ;
void getscreensize( int *widthp, int *heightp) ;

#if UNIX
# include <signal.h>
# ifdef SIGWINCH
   extern int chg_width, chg_height ;
# endif
#endif

#endif
/* end of display.h */
