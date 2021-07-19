#ifndef _BIND_H_
#define _BIND_H_

#include "fnp_t.h"

#define	APROP	1  /* Add code for Apropos command                 */

#if APROP
int apro( int f, int n) ;
#endif

int help( int f, int n) ;
int deskey( int f, int n) ;
int bindtokey( int f, int n) ;
int unbindkey( int f, int n) ;
int desbind( int f, int n) ;
int startup( const char *fname) ;
fnp_t getbind( unsigned keycode) ;
const char *transbind( char *skey) ;

#endif
