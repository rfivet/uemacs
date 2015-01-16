#ifndef _BIND_H_
#define _BIND_H_

#define	APROP	1  /* Add code for Apropos command                 */

#if APROP
int apro( int f, int n) ;
int strinc( char *source, char *sub) ;
#endif

/* Some global fuction declarations. */
typedef int (*fn_t)(int, int);

int help( int f, int n) ;
int deskey( int f, int n) ;
int bindtokey( int f, int n) ;
int unbindkey( int f, int n) ;
int unbindchar( int c) ;
int desbind( int f, int n) ;
int buildlist( int type, char *mstring) ;
unsigned int getckey( int mflag) ;
int startup( const char *fname) ;
void cmdstr( int c, char *seq) ;
fn_t getbind( int c) ;
fn_t fncmatch( char *) ;
unsigned int stock( char *keyname) ;
char *transbind( char *skey) ;

#endif
