#ifndef _RANDOM_H_
#define _RANDOM_H_

#define AEDIT 1

extern int fillcol ;	/* Fill column                  */


/* Uninitialized global external declarations. */

#define CFCPCN  0x0001	/* Last command was C-P, C-N    */
#define CFKILL  0x0002	/* Last command was a kill      */

extern int thisflag ;	/* Flags, this command          */
extern int lastflag ;	/* Flags, last command          */

int setfillcol( int f, int n) ;
int showcpos( int f, int n) ;
int getcline( void) ;
int getccol( int bflg) ;
int setccol( int pos) ;
int twiddle( int f, int n) ;
int quote( int f, int n) ;
int insert_tab( int f, int n) ;
#if AEDIT
int detab( int f, int n) ;
int entab( int f, int n) ;
int trim( int f, int n) ;
#endif
int openline( int f, int n) ;
int insert_newline( int f, int n) ;
int cinsert( void) ;
int insbrace( int n, int c) ;
int inspound( void) ;
int deblank( int f, int n) ;
int indent( int f, int n) ;
int forwdel( int f, int n) ;
int backdel( int f, int n) ;
int killtext( int f, int n) ;
int setemode( int f, int n) ;
int delmode( int f, int n) ;
int setgmode( int f, int n) ;
int delgmode( int f, int n) ;
int adjustmode( int kind, int global) ;
int clrmes( int f, int n) ;
int writemsg( int f, int n) ;
int getfence( int f, int n) ;
int fmatch( int ch) ;
int istring( int f, int n) ;
int ovstring( int f, int n) ;

#endif
