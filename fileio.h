#ifndef _FILEIO_H_
#define _FILEIO_H_

#define FTYPE_NONE  0
#define FTYPE_UNIX  1
#define FTYPE_DOS   2
#define FTYPE_MAC   4

extern int ftype ;

int fexist( const char *fname) ;
int ffclose( void) ;
int ffgetline( void) ;
int ffputline( char *buf, int nbuf, int dosflag) ;
int ffropen( const char *fn) ;
int ffwopen( const char *fn) ;

#endif
