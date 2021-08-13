/* bind.h -- bindable functions dealing with name and key bindings */
#ifndef _BIND_H_
#define _BIND_H_

#include "names.h"  /* BINDABLE() */

/* Bindable uEMACS functions */
BINDABLE( apro) ;
BINDABLE( bindtokey) ;
BINDABLE( desbind) ;
BINDABLE( deskey) ;
BINDABLE( help) ;
BINDABLE( unbindkey) ;

int startup( const char *fname) ;

/* find a key to function association in the key to function mapping table */
const char *transbind( char *skey) ;    /* by string representation of key */

#endif
/* end of bind.h */
