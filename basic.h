/* basic.h -- basic commands for cursor movement in active window */
#ifndef _BASIC_H_
#define _BASIC_H_

#include "names.h"  /* BINDABLE() */

/* $overlap is the size of the line overlap when kbd calls page forw/back
   if 0, page will move by 2/3 of the window size (1/3 page overlap)
   default to 0
 */
extern int overlap ;    /* $overlap: line overlap in forw/back page */


/* $target (== curgoal) is the column target when doing line move */
extern int curgoal ;    /* $target: Goal for C-P previous-line, C-N next-line */


/* Bindable functions */
BBINDABLE( backline) ;
TBINDABLE( backpage) ;
BBINDABLE( forwline) ;
TBINDABLE( forwpage) ;
TBINDABLE( gotobob) ;
TBINDABLE( gotobol) ;
TBINDABLE( gotoeob) ;
TBINDABLE( gotoeol) ;
 BINDABLE( gotoline) ;
TBINDABLE( setmark) ;
TBINDABLE( swapmark) ;

#endif
/* end of basic.h */
