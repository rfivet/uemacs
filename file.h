#ifndef _FILE_H_
#define _FILE_H_

#include "buffer.h"
#include "crypt.h"
#include "retcode.h"

#if CRYPT
void cryptbufferkey( struct buffer *bp) ;
int  set_encryption_key( int f, int n) ;
#endif

extern boolean restflag ;		/* restricted use?              */

int fileread( int f, int n) ;
int insfile( int f, int n) ;
int filefind( int f, int n) ;
int viewfile( int f, int n) ;
int getfile( const char *fname, boolean lockfl) ;
int readin( const char *fname, boolean lockfl) ;
void makename( bname_t bname, const char *fname) ;
void unqname( char *name) ;
int filewrite( int f, int n) ;
int filesave( int f, int n) ;
int writeout( const char *fn) ;
int filename( int f, int n) ;
int ifile( const char *fname) ;

#endif
