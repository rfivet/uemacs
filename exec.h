/* exec.h -- bindable functions to execute functions, macros and procedures */
#ifndef _EXEC_H_
#define _EXEC_H_

#include "names.h"

extern boolean clexec ; /* command line execution flag  */

int dofile( const char *fname) ;
void gettoken( char *tok, int maxtoksize) ;
boolean gettokval( char *tok, int maxtoksize) ;
char *getnewtokval( void) ;

/* Bindable functions */
BINDABLE( execbuf) ;
BINDABLE( execcmd) ;
BINDABLE( execfile) ;
BINDABLE( execproc) ;
BINDABLE( namedcmd) ;
BINDABLE( storemac) ;
BINDABLE( storeproc) ;
BINDABLE( cbuf1) ;
BINDABLE( cbuf2) ;
BINDABLE( cbuf3) ;
BINDABLE( cbuf4) ;
BINDABLE( cbuf5) ;
BINDABLE( cbuf6) ;
BINDABLE( cbuf7) ;
BINDABLE( cbuf8) ;
BINDABLE( cbuf9) ;
BINDABLE( cbuf10) ;
BINDABLE( cbuf11) ;
BINDABLE( cbuf12) ;
BINDABLE( cbuf13) ;
BINDABLE( cbuf14) ;
BINDABLE( cbuf15) ;
BINDABLE( cbuf16) ;
BINDABLE( cbuf17) ;
BINDABLE( cbuf18) ;
BINDABLE( cbuf19) ;
BINDABLE( cbuf20) ;
BINDABLE( cbuf21) ;
BINDABLE( cbuf22) ;
BINDABLE( cbuf23) ;
BINDABLE( cbuf24) ;
BINDABLE( cbuf25) ;
BINDABLE( cbuf26) ;
BINDABLE( cbuf27) ;
BINDABLE( cbuf28) ;
BINDABLE( cbuf29) ;
BINDABLE( cbuf30) ;
BINDABLE( cbuf31) ;
BINDABLE( cbuf32) ;
BINDABLE( cbuf33) ;
BINDABLE( cbuf34) ;
BINDABLE( cbuf35) ;
BINDABLE( cbuf36) ;
BINDABLE( cbuf37) ;
BINDABLE( cbuf38) ;
BINDABLE( cbuf39) ;
BINDABLE( cbuf40) ;

#endif
/* end of exec.h */
