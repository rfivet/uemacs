/* execute.h -- */

#define	CFENCE	1  /* fence matching in CMODE */


#include "names.h"		/* key code */


extern int gasave ;		/* global ASAVE size            */
extern int gacount ;	/* count until next ASAVE       */


int execute( unsigned keycode, int f, int n) ;
void kbd_loop( void) ;

#if CFENCE
BINDABLE( getfence) ;
#endif

/* end of execute.h */
