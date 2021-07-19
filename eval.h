#ifndef _EVAL_H_
#define _EVAL_H_


#define DEBUGM	1		/* $debug triggers macro debugging	*/

#if	DEBUGM
int mdbugout( char *fmt, ...) ;
#endif


extern int macbug ;		/* macro debuging flag          */
extern int cmdstatus ;		/* last command status          */
extern int rval ;		/* return value of a subprocess */
extern long envram ;		/* # of bytes current in use by malloc */

int readfirst_f( void) ;
int is_it_cmd( char *token) ;

void varinit( void) ;
int setvar( int f, int n) ;
const char *getval( char *token) ;
int stol( char *val) ;
char *mklower( char *str) ;

int clrmes( int f, int n) ;
int writemsg( int f, int n) ;

#endif
