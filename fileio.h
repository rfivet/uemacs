#ifndef _FILEIO_H_
#define _FILEIO_H_

#include "retcode.h"

typedef enum {
	FIOSUC,	/* File I/O, success.             */
	FIOFNF,	/* File I/O, file not found.      */
	FIOEOF,	/* File I/O, end of file.         */
	FIOERR,	/* File I/O, error.               */
	FIOMEM,	/* File I/O, out of memory        */
	FIOFUN	/* File I/O, eod of file/bad line */
} fio_code ;

#define FTYPE_NONE  0
#define FTYPE_UNIX  1
#define FTYPE_DOS   2
#define FTYPE_MAC   4
/*      FTYPE_MIXED [ 3, 5, 6, 7] */

#ifndef CRYPT
#error CRYPT should be defined.
#elif CRYPT
extern boolean	is_crypted ;	/* currently encrypting?   */
#endif

extern char		*fline ;		/* dynamic return line     */
extern int 		flen ;			/* current allocated length of fline */
extern int 		ftype ;
extern int		fpayload ;		/* actual length of fline content */

fio_code ffclose( void) ;
fio_code ffgetline( void) ;
fio_code ffputline( char *buf, int nbuf, int dosflag) ;
fio_code ffropen( const char *fn) ;
fio_code ffwopen( const char *fn) ;

#endif
