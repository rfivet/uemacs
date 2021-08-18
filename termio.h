/* termio.h -- */
#ifndef _TERMIO_H_
#define _TERMIO_H_

#include "utf8.h"

#define TYPEAH  1       /* type ahead causes update to be skipped */

extern int ttrow ;      /* Row location of HW cursor    */
extern int ttcol ;      /* Column location of HW cursor */

void ttopen( void) ;
void ttclose( void) ;
int ttputc( unicode_t c) ;
void ttflush( void) ;
int ttgetc( void) ;
int typahead( void) ;

#endif
/* end of termio.h */
