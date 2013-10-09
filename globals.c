/* globals.c -- implements globals.h */
#include "globals.h"

#include "defines.h"
#include "retcode.h"

/* initialized global definitions */

int fillcol = 72;		/* Current fill column          */


int clexec = FALSE;		/* command line execution flag  */

int metac = CONTROL | '[';	/* current meta character */
int ctlxc = CONTROL | 'X';	/* current control X prefix char */
int reptc = CONTROL | 'U';	/* current universal repeat char */
int abortc = CONTROL | 'G';	/* current abort command char   */

int tabmask = 0x07;		/* tabulator mask */


int rval = 0;			/* return value of a subprocess */

/* uninitialized global definitions */

int thisflag;			/* Flags, this command          */
int lastflag;			/* Flags, last command          */
int curgoal;			/* Goal for C-P, C-N            */
