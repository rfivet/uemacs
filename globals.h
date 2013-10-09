/* globals.h -- Global variable definitions */

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

/* Initialized global external declarations. */

extern int fillcol;		/* Fill column                  */


extern int clexec;		/* command line execution flag  */

extern int metac;		/* current meta character */
extern int ctlxc;		/* current control X prefix char */
extern int reptc;		/* current universal repeat char */
extern int abortc;		/* current abort command char   */

extern int tabmask;


extern int rval;		/* return value of a subprocess */

/* Uninitialized global external declarations. */

#define CFCPCN  0x0001		/* Last command was C-P, C-N    */
#define CFKILL  0x0002		/* Last command was a kill      */

extern int thisflag;		/* Flags, this command          */
extern int lastflag;		/* Flags, last command          */

extern int curgoal;		/* Goal for C-P, C-N            */

#endif
