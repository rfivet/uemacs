/* fileio.h -- file primitives */
#ifndef _FILEIO_H_
#define _FILEIO_H_

typedef enum {
    FIOSUC, /* File I/O, success.             */
    FIOFNF, /* File I/O, file not found.      */
    FIOEOF, /* File I/O, end of file.         */
    FIOERR, /* File I/O, error.               */
    FIOMEM  /* File I/O, out of memory        */
} fio_code ;

#define FTYPE_NONE  0
#define FTYPE_UNIX  1
#define FTYPE_DOS   2
#define FTYPE_MAC   4
/*      FTYPE_MIXED [ 3, 5, 6, 7] */

#define FCODE_ASCII 0
#define FCODE_MASK  0x80
#define FCODE_UTF_8 0x81
#define FCODE_EXTND 0x82
#define FCODE_MIXED 0x83

extern char *fline ;        /* dynamic return line     */
extern int  ftype ;
extern int  fcode ;         /* encoding type */
extern int  fpayload ;      /* actual length of fline content */

fio_code ffclose( void) ;
fio_code ffgetline( void) ;
fio_code ffputline( char *buf, int nbuf, int dosflag) ;
fio_code ffropen( const char *fn) ;
fio_code ffwopen( const char *fn) ;

#endif
/* end of fileio.h */
