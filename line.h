#ifndef LINE_H_
#define LINE_H_

#include "utf8.h"

#define NLINE   256		/* # of bytes, input line       */

/*
 * All text is kept in circularly linked lists of "struct line" structures. These
 * begin at the header line (which is the blank line beyond the end of the
 * buffer). This line is pointed to by the "struct buffer". Each line contains a the
 * number of bytes in the line (the "used" size), the size of the text array,
 * and the text. The end of line is not stored as a byte; it's implied. Future
 * additions will include update hints, and a list of marks into the line.
 */
struct line {
	struct line *l_fp;	/* Link to the next line        */
	struct line *l_bp;	/* Link to the previous line    */
	int l_size;		/* Allocated size               */
	int l_used;		/* Used size                    */
	char l_text[1];		/* A bunch of characters.       */
};

#define lforw(lp)       ((lp)->l_fp)
#define lback(lp)       ((lp)->l_bp)
#define lgetc(lp, n)    ((lp)->l_text[(n)]&0xFF)
#define lputc(lp, n, c) ((lp)->l_text[(n)]=(c))
#define llength(lp)     ((lp)->l_used)

extern int tabmask ;

char *getkill( void) ;

int backchar( int f, int n) ;
int forwchar( int f, int n) ;

void lfree( struct line *lp) ;
void lchange( int flag) ;
int insspace( int f, int n) ;
int linstr( char *instr) ;
int linsert( int n, int c) ;
int lover( char *ostr) ;
int lnewline( void) ;
int ldelete( long n, int kflag) ;
int ldelchar( long n, int kflag) ;
int lgetchar( unicode_t *) ;
char *getctext( void) ;
void kdelete( void) ;
int kinsert( int c) ;
int yank( int f, int n) ;
struct line *lalloc( int) ;  /* Allocate a line. */

#endif  /* LINE_H_ */
