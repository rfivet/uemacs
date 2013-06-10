#ifndef _FILEIO_H_
#define _FILEIO_H_

#include "estruct.h"

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

extern char *fline ;	/* dynamic return line */
extern int flen ;	/* current length of fline */
extern int ftype ;

boolean fexist( const char *fname) ;
fio_code ffclose( void) ;
fio_code ffgetline( void) ;
fio_code ffputline( unsigned char *buf, int nbuf, int dosflag) ;
fio_code ffropen( const char *fn) ;
fio_code ffwopen( const char *fn) ;

#endif
