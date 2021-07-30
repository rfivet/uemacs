#ifndef _BIND_H_
#define _BIND_H_

#include "names.h"

#define	APROP	1  /* Add code for Apropos command                 */

/* uEMACS functions */
#if APROP
int apro( int f, int n) ;
#endif

int help( int f, int n) ;
int deskey( int f, int n) ;
int bindtokey( int f, int n) ;
int unbindkey( int f, int n) ;
int desbind( int f, int n) ;

int startup( const char *fname) ;

/* find a key to function association in the key to function mapping table */
kbind_p getkeybind( unsigned keycode) ;	/* by key code */
const char *transbind( char *skey) ;	/* by string representation of key */

#endif
