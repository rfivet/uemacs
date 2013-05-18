#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "estruct.h"

int reposition( int f, int n);
int redraw( int f, int n) ;
int nextwind( int f, int n) ;
int prevwind( int f, int n) ;
int mvdnwind( int f, int n) ;
int mvupwind( int f, int n) ;
int onlywind( int f, int n) ;
int delwind( int f, int n) ;
int splitwind( int f, int n) ;
int enlargewind( int f, int n) ;
int shrinkwind( int f, int n) ;
int resize( int f, int n) ;
int scrnextup( int f, int n) ;
int scrnextdw( int f, int n) ;
int savewnd( int f, int n) ;
int restwnd( int f, int n) ;
int newsize( int f, int n) ;
int newwidth( int f, int n) ;
int getwpos( void) ;
void cknewwindow( void) ;
struct window *wpopup( void) ;  /* Pop up window creation. */

#endif
