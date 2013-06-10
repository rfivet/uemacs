#ifndef _BIND_H_
#define _BIND_H_

#include "edef.h"

int help( int f, int n) ;
int deskey( int f, int n) ;
int bindtokey( int f, int n) ;
int unbindkey( int f, int n) ;
int unbindchar( int c) ;
int desbind( int f, int n) ;
int apro( int f, int n) ;
int buildlist( int type, char *mstring) ;
int strinc( char *source, char *sub) ;
unsigned int getckey( int mflag) ;
int startup( char *sfname) ;
void cmdstr( int c, char *seq) ;
fn_t getbind( int c) ;
fn_t fncmatch( char *) ;
unsigned int stock( char *keyname) ;
char *transbind( char *skey) ;

#endif
