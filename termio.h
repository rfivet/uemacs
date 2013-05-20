#ifndef _TERMIO_H_
#define _TERMIO_H_

void ttopen( void) ;
void ttclose( void) ;
int ttputc( int c) ;
void ttflush( void) ;
int ttgetc( void) ;
int typahead( void) ;

#endif
