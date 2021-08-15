/* flook.h -- */
#ifndef _FLOOK_H_
#define _FLOOK_H_

#include "retcode.h"    /* boolean */

#define rcfname     pathname[ 0]
#define hlpfname    pathname[ 1]

extern const char *pathname[] ;


boolean fexist( const char *fname) ;
char *flook( const char *fname, boolean hflag) ;

#endif
/* end of flook.h */
