#ifndef _SEARCH_H_
#define _SEARCH_H_

#define MAGIC	1	/* include regular expression matching? */

#include "line.h"

/*
 * PTBEG, PTEND, FORWARD, and REVERSE are all toggle-able values for
 * the scan routines.
 */
#define	PTBEG	0		/* Leave the point at the beginning on search   */
#define	PTEND	1		/* Leave the point at the end on search         */
#define	FORWARD	0		/* forward direction            */
#define REVERSE	1		/* backwards direction          */

int scanner( const char *patrn, int direct, int beg_or_end) ;

int forwsearch( int f, int n) ;
int forwhunt( int f, int n) ;
int backsearch( int f, int n) ;
int backhunt( int f, int n) ;
int eq( unsigned char bc, unsigned char pc) ;
void savematch( void) ;
void rvstrcpy( char *rvstr, char *str) ;
int sreplace( int f, int n) ;
int qreplace( int f, int n) ;
int delins( int dlength, char *instr, int use_meta) ;
int expandp( char *srcstr, char *deststr, int maxlength) ;
int boundry( struct line *curline, int curoff, int dir) ;

#if MAGIC
void mcclear( void) ;
void rmcclear( void) ;
#endif

#endif
