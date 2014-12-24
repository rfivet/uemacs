#ifndef _PKLOCK_H_
#define _PKLOCK_H_

#ifndef _ESTRUCT_H_
#error uEmacs compilation settings needs to be done!
#endif

#if (FILOCK && BSD) || SVR4

char *dolock( const char *fname) ;
char *undolock( const char *fname) ;

#endif

#endif
