/*	edef.h
 *
 *	Global variable definitions
 *
 *	written by Dave G. Conroy
 *	modified by Steve Wilhite, George Jones
 *	greatly modified by Daniel Lawrence
 *	modified by Petri Kutvonen
 */
#ifndef EDEF_H_
#define EDEF_H_

#include "buffer.h"
#include "estruct.h"

#include <stdlib.h>
#include <string.h>

/* Some global fuction declarations. */
typedef int (*fn_t)(int, int);

/* Initialized global external declarations. */

extern int fillcol;		/* Fill column                  */
extern char pat[];		/* Search pattern               */
extern char rpat[];		/* Replacement pattern          */
extern char tap[];	    /* Reversed pattern array.      */
extern char *execstr;	/* pointer to string to execute */


extern int eolexist;		/* does clear to EOL exist?     */
extern int revexist;		/* does reverse video exist?    */
extern int flickcode;		/* do flicker supression?       */
extern const char *modename[];	/* text names of modes          */
extern const char *mode2name[];	/* text names of modes          */
extern const char modecode[];	/* letters to represent modes   */
extern int gmode;		/* global editor mode           */
extern int gflags;		/* global control flag          */
extern int gfcolor;		/* global forgrnd color (white) */
extern int gbcolor;		/* global backgrnd color (black) */
extern int gasave;		/* global ASAVE size            */
extern int gacount;		/* count until next ASAVE       */
extern int sgarbf;		/* State of screen unknown      */
extern int mpresf;		/* Stuff in message line        */
extern int clexec;		/* command line execution flag  */
extern int mstore;		/* storing text to macro flag   */
extern int discmd;		/* display command flag         */
extern int disinp;		/* display input characters     */

extern int vtrow;		/* Row location of SW cursor */
extern int vtcol;		/* Column location of SW cursor */
extern int lbound;		/* leftmost column of current line
				   being displayed */
extern int taboff;		/* tab offset for display       */
extern int metac;		/* current meta character */
extern int ctlxc;		/* current control X prefix char */
extern int reptc;		/* current universal repeat char */
extern int abortc;		/* current abort command char   */

extern int quotec;		/* quote char during mlreply() */
extern int tabmask;
extern char *cname[];		/* names of colors              */


#if 0
#define	STOP	0		/* keyboard macro not in use    */
#define	PLAY	1		/*                playing       */
#define	RECORD	2		/*                recording     */
#endif

typedef enum {
	STOP, PLAY, RECORD
} kbdstate ;
extern kbdstate kbdmode ;		/* current keyboard macro mode  */
extern int kbdrep;		/* number of repetitions        */
extern int restflag;		/* restricted use?              */
extern int lastkey;		/* last keystoke                */
extern int seed;		/* random number seed           */
extern long envram;		/* # of bytes current in use by malloc */
extern int macbug;		/* macro debuging flag          */
extern char errorm[];		/* error literal                */
extern char truem[];		/* true literal                 */
extern char falsem[];		/* false litereal               */
extern int cmdstatus;		/* last command status          */
extern char palstr[];		/* palette string               */
extern int saveflag;		/* Flags, saved with the $target var */
extern int rval;		/* return value of a subprocess */
#if	PKCODE
extern int justflag;		/* justify, don't fill */
#endif
extern int overlap;		/* line overlap in forw/back page */
extern int scrollcount;		/* number of lines to scroll */

/* Uninitialized global external declarations. */

extern int currow;		/* Cursor row                   */
extern int curcol;		/* Cursor column                */


#define CFCPCN  0x0001		/* Last command was C-P, C-N    */
#define CFKILL  0x0002		/* Last command was a kill      */

extern int thisflag;		/* Flags, this command          */
extern int lastflag;		/* Flags, last command          */

extern int curgoal;		/* Goal for C-P, C-N            */
extern struct window *curwp;		/* Current window               */
extern struct buffer *curbp;		/* Current buffer               */
extern struct window *wheadp;                /* Head of list of windows      */
extern struct buffer *bheadp;		/* Head of list of buffers      */
extern struct buffer *blistp;		/* Buffer for C-X C-B           */

extern char sres[NBUFN];	        /* Current screen resolution.   */

extern unsigned int matchlen;
extern unsigned int mlenold;
extern char *patmatch;
extern struct line *matchline;
extern int matchoff;

#if	DEBUGM
/* Vars needed for macro debugging output. */
extern char outline[];		/* Global string to hold debug line text. */
#endif

#endif  /* EDEF_H_ */
