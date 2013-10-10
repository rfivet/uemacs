/* globals.c -- implements globals.h */
#include "globals.h"

#include "defines.h"

/* initialized global definitions */

int metac = CONTROL | '[';	/* current meta character */
int ctlxc = CONTROL | 'X';	/* current control X prefix char */
int reptc = CONTROL | 'U';	/* current universal repeat char */
int abortc = CONTROL | 'G';	/* current abort command char   */

/* uninitialized global definitions */

int thisflag;			/* Flags, this command          */
int lastflag;			/* Flags, last command          */
int curgoal;			/* Goal for C-P, C-N            */
