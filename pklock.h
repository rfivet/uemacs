#ifndef _PKLOCK_H_
#define _PKLOCK_H_

#if (FILOCK && BSD) || SVR4

char *dolock( char *fname) ;
char *undolock( char *fname) ;

#endif

#endif
