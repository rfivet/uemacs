#ifndef _BASIC_H_
#define _BASIC_H_

extern int overlap ;		/* line overlap in forw/back page	*/
extern int curgoal ;		/* Goal for C-P, C-N			*/


int gotobol( int f, int n) ;
int gotoeol( int f, int n) ;
int gotoline( int f, int n) ;
int gotobob( int f, int n) ;
int gotoeob( int f, int n) ;
int forwline( int f, int n) ;
int backline( int f, int n) ;
int forwpage( int f, int n) ;
int backpage( int f, int n) ;
int setmark( int f, int n) ;
int swapmark( int f, int n) ;

#endif
