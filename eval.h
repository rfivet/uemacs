/* eval.h -- variables and operands evaluation */
#ifndef _EVAL_H_
# define _EVAL_H_

#include "names.h"

#define DEBUGM  1       /* $debug triggers macro debugging  */

# if    DEBUGM
int mdbugout( char *fmt, ...) ;
# endif

extern int macbug ;     /* macro debuging flag          */
extern int cmdstatus ;  /* last command status          */
extern int rval ;       /* return value of a subprocess */
extern long envram ;    /* # of bytes current in use by malloc */

int readfirst_f( void) ;
int is_it_cmd( char *token) ;

void varinit( void) ;
const char *getval( char *token) ;
int stol( char *val) ;
char *mklower( char *str) ;

/* Bindable functions */
TBINDABLE( clrmes) ;
BINDABLE( setvar) ;
BINDABLE( writemsg) ;

#endif

/* end of eval.h */
