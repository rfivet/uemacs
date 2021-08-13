/* spawn.h -- various operating system access commands */
#ifndef _SPAWN_H_
#define _SPAWN_H_

#include "names.h"  /* BINDABLE() */

/* Bindable functions */
BINDABLE( spawncli) ;
BINDABLE( bktoshell) ;
BINDABLE( spawn) ;
BINDABLE( execprg) ;
BINDABLE( pipecmd) ;
BINDABLE( filter_buffer) ;

void rtfrmshell( void) ;

#endif
/* end of spawn.h */
