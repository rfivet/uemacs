#ifndef _TERMIO_H_
#define _TERMIO_H_

extern int ttrow ;		/* Row location of HW cursor */
extern int ttcol ;		/* Column location of HW cursor */

void ttopen( void) ;
void ttclose( void) ;
int ttputc( int c) ;
void ttflush( void) ;
int ttgetc( void) ;
int typahead( void) ;

#endif
