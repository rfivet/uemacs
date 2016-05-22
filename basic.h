/* basic.h -- basic commands for cursor movement in active window */

#ifndef _BASIC_H_
#define _BASIC_H_

#include "retcode.h"

/*
** $overlap is the size of the line overlap when doing page forw/back
** if 0, page will move by 2/3 of the window size (1/3 page overlap)
** default to 0
*/
#define DEFAULT_OVERLAP	0
extern int overlap ;		/* line overlap in forw/back page	*/


extern int curgoal ;		/* Goal for C-P, C-N			*/


boolean gotobol( int f, int n) ;
boolean gotoeol( int f, int n) ;
int gotoline( int f, int n) ;
boolean gotobob( int f, int n) ;
boolean gotoeob( int f, int n) ;
boolean forwline( int f, int n) ;
boolean backline( int f, int n) ;
boolean forwpage( int f, int n) ;
boolean backpage( int f, int n) ;
boolean setmark( int f, int n) ;
boolean swapmark( int f, int n) ;

#endif

/* end of basic.h */
