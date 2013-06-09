#ifndef _FILEIO_H_
#define _FILEIO_H_

#define FIOSUC  0       /* File I/O, success.             */
#define FIOFNF  1       /* File I/O, file not found.      */
#define FIOEOF  2       /* File I/O, end of file.         */
#define FIOERR  3       /* File I/O, error.               */
#define FIOMEM  4       /* File I/O, out of memory        */
#define FIOFUN  5       /* File I/O, eod of file/bad line */

#define FTYPE_NONE  0
#define FTYPE_UNIX  1
#define FTYPE_DOS   2
#define FTYPE_MAC   4

extern int ftype ;

int fexist( const char *fname) ;
int ffclose( void) ;
int ffgetline( void) ;
int ffputline( char *buf, int nbuf, int dosflag) ;
int ffropen( const char *fn) ;
int ffwopen( const char *fn) ;

#endif
