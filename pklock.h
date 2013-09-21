#ifndef _PKLOCK_H_
#define _PKLOCK_H_

#if (FILOCK && BSD) || SVR4

char *dolock( const char *fname) ;
char *undolock( const char *fname) ;

#endif

#endif
