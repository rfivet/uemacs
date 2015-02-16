#ifndef _BIND_H_
#define _BIND_H_

#define	APROP	1  /* Add code for Apropos command                 */

#if APROP
int apro( int f, int n) ;
#endif

/* Some global fuction declarations. */
typedef int (*fn_t)(int, int);

int help( int f, int n) ;
int deskey( int f, int n) ;
int bindtokey( int f, int n) ;
int unbindkey( int f, int n) ;
int desbind( int f, int n) ;
int startup( const char *fname) ;
fn_t getbind( int c) ;
fn_t fncmatch( char *) ;
char *transbind( char *skey) ;

#endif
