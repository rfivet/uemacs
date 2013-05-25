#ifndef _LOCK_H_
#define _LOCK_H_

#if	BSD | SVR4

int lockchk( char *fname) ;
int lockrel( void) ;
int lock( char *fname) ;
int unlock( char *fname) ;
void lckerror( char *errstr) ;

#endif

#endif
