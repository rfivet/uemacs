#ifndef _ESTRUCT_H_
#define _ESTRUCT_H_

/*      ESTRUCT.H
 *
 *      Structure and preprocessor defines
 *
 *	written by Dave G. Conroy
 *	modified by Steve Wilhite, George Jones
 *      substantially modified by Daniel Lawrence
 *	modified by Petri Kutvonen
 */


#define MAXCOL	500
#define MAXROW	500

#ifdef	MSDOS
#undef	MSDOS
#endif
#ifdef	EGA
#undef	EGA
#endif
#ifdef	CTRLZ
#undef	CTRLZ
#endif

/* Machine/OS definitions. */

#if defined(AUTOCONF) || defined(MSDOS) || defined(BSD) || defined(SYSV) || defined(VMS)

/* Make an intelligent guess about the target system. */

#if defined(__TURBOC__)
#define MSDOS 1 /* MS/PC DOS 3.1-4.0 with Turbo C 2.0 */
#else
#define	MSDOS 0
#endif

#if defined(BSD) || defined(sun) || defined(ultrix) || (defined(vax) && defined(unix)) || defined(ultrix) || defined(__osf__)
#ifndef BSD
#define BSD 1 /* Berkeley UNIX */
#endif
#else
#define	BSD 0
#endif

#if defined(SVR4) || defined(__linux__)	/* ex. SunOS 5.3 */
#define SVR4 1
#define SYSV 1
#undef BSD
#endif

#if defined(SYSV) || defined(u3b2) || defined(_AIX) || (defined(i386) && defined(unix)) || defined(__hpux) || defined( __unix__)
#define	USG 1 /* System V UNIX */
#else
#define	USG 0
#endif

#if defined(VMS) || (defined(vax) && ! defined(unix))
#define VMS 1 /* VAX/VMS */
#else
#define VMS 0
#endif

#define	V7 0 /* No more. */

#else

#define MSDOS   1		/* MS-DOS                       */
#define V7      0		/* V7 UNIX or Coherent or BSD4.2 */
#define	BSD	0		/* UNIX BSD 4.2 and ULTRIX      */
#define	USG	0		/* UNIX system V                */
#define VMS     0		/* VAX/VMS                      */

#endif				/*autoconf */

#ifndef	AUTOCONF

/*	Compiler definitions			*/
#define	UNIX	0		/* a random UNIX compiler */
#define	MSC	0		/* MicroSoft C compiler, versions 3 up */
#define	TURBO	1		/* Turbo C/MSDOS */

#else

#define	UNIX	(V7 | BSD | USG)
#define	MSC	0
#define	TURBO	MSDOS

#endif				/*autoconf */

/*	Debugging options	*/

#define	RAMSIZE	0		/* dynamic RAM memory usage tracking */
#define	RAMSHOW	0		/* auto dynamic RAM reporting */

#ifndef	AUTOCONF

/*   Special keyboard definitions            */

#define VT220	0		/* Use keypad escapes P.K.      */
#define VT100   0		/* Handle VT100 style keypad.   */

/*	Terminal Output definitions		*/

#define ANSI    0		/* ANSI escape sequences        */
#define	VMSVT	0		/* various VMS terminal entries */
#define VT52    0		/* VT52 terminal (Zenith).      */
#define TERMCAP 0		/* Use TERMCAP                  */
#define	IBMPC	1		/* IBM-PC CGA/MONO/EGA driver   */

#else

#define	VT220	(UNIX | VMS)
#define	VT100	0

#define	ANSI	0
#define	VMSVT	VMS
#define	VT52	0
#define	TERMCAP	UNIX
#define	IBMPC	MSDOS

#endif /* Autoconf. */

/*	Configuration options	*/

#define CVMVAS  1  /* arguments to page forward/back in pages      */
#define	CLRMSG	0  /* space clears the message line with no insert */
#define	CFENCE	1  /* fench matching in CMODE                      */
#define	TYPEAH	1  /* type ahead causes update to be skipped       */
#define DEBUGM	1  /* $debug triggers macro debugging              */
#define	VISMAC	0  /* update display during keyboard macros        */
#define	CTRLZ	0  /* add a ^Z at end of files under MSDOS only    */
#define ADDCR	0  /* ajout d'un CR en fin de chaque ligne (ST520) */
#define	NBRACE	1  /* new style brace matching command             */
#define	REVSTA	1  /* Status line appears in reverse video         */

#ifndef	AUTOCONF

#define	COLOR	1  /* color commands and windows                   */
#define	FILOCK	0  /* file locking under unix BSD 4.2              */

#else

#define	COLOR	MSDOS
#ifdef  SVR4
#define FILOCK  1
#else
#define	FILOCK	BSD
#endif

#endif /* Autoconf. */

#if 0
#define	ISRCH	1  /* Incremental searches like ITS EMACS          */
#define	WORDPRO	1  /* Advanced word processing features            */
#define	APROP	1  /* Add code for Apropos command                 */
#define	CRYPT	1  /* file encryption enabled?                     */
#define MAGIC	1  /* include regular expression matching?         */
#define	AEDIT	1  /* advanced editing options: en/detabbing       */
#define	PROC	1  /* named procedures                             */
#endif
#define	CLEAN	0  /* de-alloc memory on exit                      */

#define ASCII	1  /* always using ASCII char sequences for now    */
#define EBCDIC	0  /* later IBM mainfraim versions will use EBCDIC */

#ifndef	AUTOCONF

#define	XONXOFF	0  /* don't disable XON-XOFF flow control P.K.     */
#define	NATIONL	0  /* interprete [,],\,{,},| as characters P.K.    */

#else

#define	XONXOFF	(UNIX | VMS)
#define	NATIONL	(UNIX | VMS)

#endif /* Autoconf. */

#define	PKCODE	1      /* include my extensions P.K., define always    */
#define	IBMCHR	MSDOS  /* use IBM PC character set P.K.                */
#define SCROLLCODE 1   /* scrolling code P.K.                          */

/* System dependant library redefinitions, structures and includes. */

#if TURBO
#include <dos.h>
#include <mem.h>
#undef peek
#undef poke
#define       peek(a,b,c,d)   movedata(a,b,FP_SEG(c),FP_OFF(c),d)
#define       poke(a,b,c,d)   movedata(FP_SEG(c),FP_OFF(c),a,b,d)
#endif

#if	VMS
#define	atoi	xatoi
#define	abs	xabs
#define	getname	xgetname
#endif

#if MSDOS & MSC
#include	<dos.h>
#include	<memory.h>
#define	peek(a,b,c,d)	movedata(a,b,FP_SEG(c),FP_OFF(c),d)
#define	poke(a,b,c,d)	movedata(FP_SEG(c),FP_OFF(c),a,b,d)
#define	movmem(a, b, c)		memcpy(b, a, c)
#endif

#if	VMS
#define	unlink(a)	delete(a)
#endif

/* Define some ability flags. */

#if	IBMPC
#define	MEMMAP	1
#else
#define	MEMMAP	0
#endif

#if	MSDOS | V7 | USG | BSD
#define	ENVFUNC	1
#else
#define	ENVFUNC	0
#endif

/* Emacs global flag bit definitions (for gflags). */

#define	GFREAD	1

/* Internal constants. */

#define	NBINDS	256		/* max # of bound keys          */
#define NFILEN  80		/* # of bytes, file name        */
#define NBUFN   16		/* # of bytes, buffer name      */
#define NLINE   256		/* # of bytes, input line       */
#define	NSTRING	128		/* # of bytes, string buffers   */
#define NKBDM   256		/* # of strokes, keyboard macro */
#define NPAT    128		/* # of bytes, pattern          */
#define HUGE    1000		/* Huge number                  */
#define	NLOCKS	100		/* max # of file locks active   */
#define	NCOLORS	8		/* number of supported colors   */

#define CONTROL 0x10000000	/* Control flag, or'ed in       */
#define META    0x20000000	/* Meta flag, or'ed in          */
#define CTLX    0x40000000	/* ^X flag, or'ed in            */
#define	SPEC	0x80000000	/* special key (function keys)  */

#include "retcode.h"

#define	BELL	0x07		/* a bell character             */
#define	TAB	0x09		/* a tab character              */

#define	INTWIDTH	sizeof(int) * 3

/*	Internal defined functions					*/

#define	nextab(a)	(a & ~tabmask) + (tabmask+1)
#ifdef	abs
#undef	abs
#endif

/* DIFCASE represents the integer difference between upper
   and lower case letters.  It is an xor-able value, which is
   fortunate, since the relative positions of upper to lower
   case letters is the opposite of ascii in ebcdic.
*/

#ifdef	islower
#undef	islower
#endif

#if	PKCODE
#ifdef	isupper
#undef	isupper
#endif
#endif

#if	ASCII

#define	DIFCASE		0x20

#if	NATIONL
#define LASTUL ']'
#define LASTLL '}'
#else
#define LASTUL 'Z'
#define LASTLL 'z'
#endif

#if	IBMCHR

#define isletter(c)	(('a' <= c && LASTLL >= c) || ('A' <= c && LASTUL >= c) || (128<=c && c<=167))
#define islower(c)	(('a' <= c && LASTLL >= c))
#define isupper(c)	(('A' <= c && LASTUL >= c))

#else

#define isletter(c)	isxletter((0xFF & (c)))
#define islower(c)	isxlower((0xFF & (c)))
#define isupper(c)	isxupper((0xFF & (c)))

#define isxletter(c)	(('a' <= c && LASTLL >= c) || ('A' <= c && LASTUL >= c) || (192<=c && c<=255))
#define isxlower(c)	(('a' <= c && LASTLL >= c) || (224 <= c && 252 >= c))
#define isxupper(c)	(('A' <= c && LASTUL >= c) || (192 <= c && 220 >= c))

#endif

#endif

#if	EBCDIC

#define	DIFCASE		0x40
#define isletter(c)	(('a' <= c && 'i' >= c) || ('j' <= c && 'r' >= c) || ('s' <= c && 'z' >= c) || ('A' <= c && 'I' >= c) || ('J' <= c && 'R' >= c) || ('S' <= c && 'Z' >= c))
#define islower(c)	(('a' <= c && 'i' >= c) || ('j' <= c && 'r' >= c) || ('s' <= c && 'z' >= c))
#if	PKCODE
#define isupper(c)	(('A' <= c && 'I' >= c) || ('J' <= c && 'R' >= c) || ('S' <= c && 'Z' >= c))
#endif

#endif

/*	Dynamic RAM tracking and reporting redefinitions	*/

#if	RAMSIZE
#define	malloc	allocate
#define	free	release
#endif

/*	De-allocate memory always on exit (if the operating system or
	main program can not
*/

#if	CLEAN
#define	exit(a)	cexit(a)

int cexit( int status) ;
#endif

#endif
