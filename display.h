/* display.h -- display functionality */
#ifndef _DISPLAY_H_
# define _DISPLAY_H_

# include <stdarg.h>

# include "estruct.h"
# include "names.h"		/* BINDABLE() */
# include "utf8.h"		/* unicode_t */

extern int mpresf ;		/* Stuff in message line */
extern int scrollcount ;	/* number of lines to scroll */
extern int discmd ;		/* display command flag         */
extern int disinp ;		/* display input characters (echo)	*/
extern int gfcolor ;		/* global forgrnd color (white) */
extern int gbcolor ;		/* global backgrnd color (black) */

/* Bindable functions */
BINDABLE( upscreen) ;

void vtinit( void) ;
void vtfree( void) ;
void vttidy( void) ;
int update( boolean force_f) ;
void updpos( void) ;
void upddex( void) ;
void updgar( void) ;
int updupd( int force) ;
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

# if UNIX
#  include <signal.h>
#  ifdef SIGWINCH
extern int chg_width, chg_height ;

void sizesignal( int signr) ;
#  endif
# endif
#endif
/* end of display.h */
