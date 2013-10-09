/* globals.c -- implements global.h */
#include "globals.h"

#include "defines.h"
#include "retcode.h"

/* initialized global definitions */

int fillcol = 72;		/* Current fill column          */


int eolexist = TRUE;		/* does clear to EOL exist      */
int revexist = FALSE;		/* does reverse video exist?    */
int flickcode = FALSE;		/* do flicker supression?       */
int sgarbf = TRUE;		/* TRUE if screen is garbage    */
int clexec = FALSE;		/* command line execution flag  */
int discmd = TRUE;		/* display command flag         */
int disinp = TRUE;		/* display input characters     */

int metac = CONTROL | '[';	/* current meta character */
int ctlxc = CONTROL | 'X';	/* current control X prefix char */
int reptc = CONTROL | 'U';	/* current universal repeat char */
int abortc = CONTROL | 'G';	/* current abort command char   */

int tabmask = 0x07;		/* tabulator mask */


long envram = 0l;		/* # of bytes current in use by malloc */
int rval = 0;			/* return value of a subprocess */
int overlap = 0;		/* line overlap in forw/back page */
int scrollcount = 1;		/* number of lines to scroll */

/* uninitialized global definitions */

int thisflag;			/* Flags, this command          */
int lastflag;			/* Flags, last command          */
int curgoal;			/* Goal for C-P, C-N            */

char sres[NBUFN];		/* current screen resolution    */
