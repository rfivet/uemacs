#include "crypt.h"
#include "defines.h"

/* #include "estruct.h" */
#include "edef.h"

/* initialized global definitions */

int fillcol = 72;		/* Current fill column          */
char *execstr = NULL;		/* pointer to string to execute */


int eolexist = TRUE;		/* does clear to EOL exist      */
int revexist = FALSE;		/* does reverse video exist?    */
int flickcode = FALSE;		/* do flicker supression?       */
int gmode = 0;			/* global editor mode           */
int gflags = GFREAD;		/* global control flag          */
int gasave = 256;		/* global ASAVE size            */
int gacount = 256;		/* count until next ASAVE       */
int sgarbf = TRUE;		/* TRUE if screen is garbage    */
int clexec = FALSE;		/* command line execution flag  */
int discmd = TRUE;		/* display command flag         */
int disinp = TRUE;		/* display input characters     */

int metac = CONTROL | '[';	/* current meta character */
int ctlxc = CONTROL | 'X';	/* current control X prefix char */
int reptc = CONTROL | 'U';	/* current universal repeat char */
int abortc = CONTROL | 'G';	/* current abort command char   */

int tabmask = 0x07;		/* tabulator mask */


kbdstate kbdmode = STOP;		/* current keyboard macro mode  */
int kbdrep = 0;			/* number of repetitions        */
int restflag = FALSE;		/* restricted use?              */
int lastkey = 0;		/* last keystoke                */
long envram = 0l;		/* # of bytes current in use by malloc */
int macbug = FALSE;		/* macro debuging flag          */
int cmdstatus = TRUE;		/* last command status          */
int saveflag = 0;		/* Flags, saved with the $target var */
int rval = 0;			/* return value of a subprocess */
int overlap = 0;		/* line overlap in forw/back page */
int scrollcount = 1;		/* number of lines to scroll */

/* uninitialized global definitions */

int thisflag;			/* Flags, this command          */
int lastflag;			/* Flags, last command          */
int curgoal;			/* Goal for C-P, C-N            */
struct window *curwp;		/* Current window               */
struct buffer *curbp;			/* Current buffer               */
struct window *wheadp;		/* Head of list of windows      */
struct buffer *bheadp;			/* Head of list of buffers      */
struct buffer *blistp;			/* Buffer for C-X C-B           */

char sres[NBUFN];		/* current screen resolution    */
