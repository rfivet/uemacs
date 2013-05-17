#ifndef _FILEIO_H_
#define _FILEIO_H_

int fexist( const char *fname) ;
int ffclose( void) ;
int ffgetline( void) ;
int ffputline( char *buf, int nbuf) ;
int ffropen( const char *fn) ;
int ffwopen( const char *fn) ;

#endif
