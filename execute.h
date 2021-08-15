/* execute.h -- */
#ifndef _EXECUTE_H_
#define _EXECUTE_H_

#include "names.h"      /* BINDABLE() */

extern int gasave ;     /* global ASAVE size            */
extern int gacount ;    /* count until next ASAVE       */

int execute( unsigned keycode, int f, int n) ;
void kbd_loop( void) ;

#define CFENCE  1       /* fence matching in CMODE */
#if CFENCE
  BINDABLE( getfence) ;
#endif

#endif
/* end of execute.h */
