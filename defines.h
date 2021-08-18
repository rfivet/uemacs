/* defines.h -- customization based on gcc predefined macroes */
#ifndef __DEFINES_H__
#define __DEFINES_H__

#if __unix__
# define UNIX 1
# if __NetBSD__
#  define BSD 1
#  define POSIX 1
# elsif __linux__
#  define USG 1
#  define SVR4 1	/* locks */
#  define POSIX 1
# else	/* __CYGWIN__ */
#  define USG 1
//#  define POSIX 1
# endif
#else
# error Missing gcc predefined __unix__
#endif

#define NSTRING 128     /* # of bytes, string buffers   */

#define TERMCAP 1		/* UNIX */
#define XONXOFF 1		/* UNIX */

#define VISMAC  0  		/* update display during keyboard macros        */

#define MSDOS	0
#define IBMPC	MSDOS
#define COLOR	MSDOS
#define MEMMAP	IBMPC


#define FILOCK (SVR4 | BSD)
#define ENVFUNC 1		/* only two types so far (USG | BSD) */

#define PKCODE 1		/* include P.K. extensions, define always    */
#define SCROLLCODE 1	/* scrolling code P.K. */

/*  Dynamic RAM tracking and reporting redefinitions    */
#define RAMSIZE 0		/* dynamic RAM memory usage tracking */
#if RAMSIZE
# define RAMSHOW 1      /* auto dynamic RAM reporting */
# include <stdlib.h>
  void *allocate( size_t size) ;
  void release( void *ptr) ;
  
# define    malloc  allocate
# define    free    release
#endif

/* De-allocate memory always on exit (if the operating system or main
   program can not
*/
#define CLEAN 0			/* de-alloc memory on exit */
#if CLEAN
# define exit(a) cexit(a)

  void cexit( int status) ;
#endif

#endif
/* end of predefs.h */
