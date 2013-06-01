#ifndef _LOCK_H_
#define _LOCK_H_

#include "estruct.h"

#if	BSD | SVR4

int lockchk( char *fname) ;
int lockrel( void) ;
int lock( char *fname) ;
int unlock( char *fname) ;

#endif

#endif
