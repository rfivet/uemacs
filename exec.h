#ifndef _EXEC_H_
#define _EXEC_H_

#include "estruct.h"

int namedcmd( int f, int n) ;
int execcmd( int f, int n) ;
int docmd( char *cline) ;
char *token( char *src, char *tok, int size) ;
int macarg( char *tok) ;
int nextarg( char *prompt, char *buffer, int size, int terminator) ;
int storemac( int f, int n) ;
int storeproc( int f, int n) ;
int execproc( int f, int n) ;
int execbuf( int f, int n) ;
int dobuf( struct buffer *bp) ;
void freewhile( struct while_block *wp) ;
int execfile( int f, int n) ;
int dofile( char *fname) ;
int cbuf( int f, int n, int bufnum) ;
int cbuf1( int f, int n) ;
int cbuf2( int f, int n) ;
int cbuf3( int f, int n) ;
int cbuf4( int f, int n) ;
int cbuf5( int f, int n) ;
int cbuf6( int f, int n) ;
int cbuf7( int f, int n) ;
int cbuf8( int f, int n) ;
int cbuf9( int f, int n) ;
int cbuf10( int f, int n) ;
int cbuf11( int f, int n) ;
int cbuf12( int f, int n) ;
int cbuf13( int f, int n) ;
int cbuf14( int f, int n) ;
int cbuf15( int f, int n) ;
int cbuf16( int f, int n) ;
int cbuf17( int f, int n) ;
int cbuf18( int f, int n) ;
int cbuf19( int f, int n) ;
int cbuf20( int f, int n) ;
int cbuf21( int f, int n) ;
int cbuf22( int f, int n) ;
int cbuf23( int f, int n) ;
int cbuf24( int f, int n) ;
int cbuf25( int f, int n) ;
int cbuf26( int f, int n) ;
int cbuf27( int f, int n) ;
int cbuf28( int f, int n) ;
int cbuf29( int f, int n) ;
int cbuf30( int f, int n) ;
int cbuf31( int f, int n) ;
int cbuf32( int f, int n) ;
int cbuf33( int f, int n) ;
int cbuf34( int f, int n) ;
int cbuf35( int f, int n) ;
int cbuf36( int f, int n) ;
int cbuf37( int f, int n) ;
int cbuf38( int f, int n) ;
int cbuf39( int f, int n) ;
int cbuf40( int f, int n) ;

#endif
