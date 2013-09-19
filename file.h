#ifndef _FILE_H_
#define _FILE_H_

#include "retcode.h"

#ifndef CRYPT
#error CRYPT should be defined.
#elif CRYPT
void cryptbufferkey( struct buffer *bp) ;
int  set_encryption_key( int f, int n) ;
#endif

int fileread( int f, int n) ;
int insfile( int f, int n) ;
int filefind( int f, int n) ;
int viewfile( int f, int n) ;
int getfile( const char *fname, boolean lockfl) ;
int readin( const char *fname, boolean lockfl) ;
void makename( char *bname, const char *fname) ;
void unqname( char *name) ;
int filewrite( int f, int n) ;
int filesave( int f, int n) ;
int writeout( const char *fn) ;
int filename( int f, int n) ;
int ifile( const char *fname) ;

#endif
