#ifndef _EVAL_H_
#define _EVAL_H_

#include "estruct.h"

void varinit( void) ;
char *gtfun( char *fname) ;
char *gtusr( char *vname) ;
char *gtenv( char *vname) ;
char *getkill( void) ;
int setvar( int f, int n) ;
void findvar( char *var, struct variable_description *vd, int size) ;
int svar( struct variable_description *var, char *value) ;
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
