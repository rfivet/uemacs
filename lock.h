#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef _ESTRUCT_H_
#error uEmacs compilation settings needs to be done!
#endif

#if	BSD | SVR4

int lockchk( const char *fname) ;
int lockrel( void) ;
int lock( const char *fname) ;
int unlock( const char *fname) ;

#endif
#endif
