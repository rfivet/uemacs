#ifndef _EVAL_H_
#define _EVAL_H_


#define DEBUGM	1		/* $debug triggers macro debugging	*/

#if	DEBUGM
int mdbugout( char *fmt, char *s1, char *s2, char *s3) ;
#endif


extern int macbug ;		/* macro debuging flag          */
extern int cmdstatus ;		/* last command status          */
extern int flickcode ;		/* do flicker supression?       */
extern int rval ;		/* return value of a subprocess */
extern long envram ;		/* # of bytes current in use by malloc */

int readfirst_f( void) ;
int is_it_cmd( char *token) ;

void varinit( void) ;
int setvar( int f, int n) ;
char *i_to_a( int i) ;
char *getval( char *token) ;
int stol( char *val) ;
char *mklower( char *str) ;
int abs( int x) ;

#endif
