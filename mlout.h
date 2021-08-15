/* mlout.h -- message line output interface */
#ifndef __MLOUT_H__
#define __MLOUT_H__

#include "retcode.h"

extern void (*mloutfmt)( const char *, ...) ;

void mloutstr( const char *str) ;
boolean mloutfail( const char *msg) ;   /* output with BELL and return FALSE */

#endif
/* end of mlout.h */
