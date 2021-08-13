/* lock.h -- */
#ifndef _LOCK_H_
#define _LOCK_H_

#include "estruct.h"

#if BSD | SVR4
int lockchk( const char *fname) ;
int lockrel( void) ;
int lock( const char *fname) ;
int unlock( const char *fname) ;
#endif

#endif
/* end of lock.h */
