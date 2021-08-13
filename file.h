/* file.h -- file centric commands */
#ifndef _FILE_H_
#define _FILE_H_

#include "buffer.h" /* bname_t */
#include "names.h"  /* BINDABLE() */

extern boolean restflag ;       /* restricted use?              */
boolean resterr( void) ;        /* restricted error message     */

/* Bindable functions */
BINDABLE( filefind) ;
BINDABLE( fileread) ;
BINDABLE( filename) ;
BINDABLE( filesave) ;
BINDABLE( filewrite) ;
BINDABLE( insfile) ;
BINDABLE( viewfile) ;

int getfile( const char *fname, boolean lockfl) ;
int readin( const char *fname, boolean lockfl) ;
void makename( bname_t bname, const char *fname) ;
void unqname( char *name) ;
int writeout( const char *fn) ;

#endif
/* end of file.h */
