#ifndef _EVAL_H_
#define _EVAL_H_

void varinit( void) ;
char *gtfun( char *fname) ;
char *gtusr( char *vname) ;
char *gtenv( char *vname) ;
char *getkill( void) ;
int setvar( int f, int n) ;
char *itoa( int i) ;
int gettyp( char *token) ;
char *getval( char *token) ;
int stol( char *val) ;
char *ltos( int val) ;
char *mkupper( char *str) ;
char *mklower( char *str) ;
int abs( int x) ;
int ernd( void) ;
int sindex( char *source, char *pattern) ;
char *xlat( char *source, char *lookup, char *trans) ;

#endif
