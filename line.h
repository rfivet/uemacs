/* line.h -- line centric interface */

#ifndef _LINE_H_
#define _LINE_H_

#include "names.h"
#include "utf8.h"

/*
 * All text is kept in circularly linked lists of "struct line" structures.
 * These begin at the header line (which is the blank line beyond the end
 * of the buffer).  This line is pointed to by the "struct buffer".  Each
 * line contains a number of bytes in the line (the "used" size), the size
 * of the text array, and the text.  The end of line is not stored as a
 * byte; it's implied.  Future additions will include update hints, and a
 * list of marks into the line.
 */
typedef struct line {
	struct line *l_fp ;	/* Forward link to the next line		*/
	struct line *l_bp ;	/* Backward link to the previous line	*/
	int l_size ;		/* Allocated size						*/
	int l_used ;		/* Used size							*/
	char l_text[ 1] ;	/* A bunch of characters				*/
} *line_p ;

#define lforw(lp)       ((lp)->l_fp)
#define lback(lp)       ((lp)->l_bp)
#define lgetc(lp, n)    ((lp)->l_text[(n)]&0xFF)
#define lputc(lp, n, c) ((lp)->l_text[(n)]=(c))
#define llength(lp)     ((lp)->l_used)

extern int tabwidth ;	/* Map to $tab, default to 8, can be set to [1, .. */

char *getkill( void) ;

/* Bindable functions */
boolean backchar( int f, int n) ;
boolean forwchar( int f, int n) ;
BINDABLE( insspace) ;
BINDABLE( yank) ;

void lfree( line_p lp) ;
void lchange( int flag) ;
int linstr( char *instr) ;
int linsert( int n, unicode_t c) ;
boolean linsert_byte( int n, int c) ;
int lover( char *ostr) ;
int lnewline( void) ;
boolean ldelete(  long n, boolean kflag) ;
boolean ldelchar( long n, boolean kflag) ;
int lgetchar( unicode_t *cref) ;
char *getctext( void) ;
void kdelete( void) ;
int kinsert( int c) ;
line_p lalloc( int minsize) ;  /* Allocate a line of at least minsize chars. */

boolean rdonly( void) ;		/* Read Only error message */

#endif  /* _LINE_H_ */

/* end of line.h */
