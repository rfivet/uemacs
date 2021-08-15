/* flook.c -- implements flook.h */
#include "flook.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"


/* possible names and paths of help files under different OSes */
const char *pathname[] = {
#if BSD | USG
    ".emacsrc",
    "emacs.hlp",
# if PKCODE
    "/usr/global/lib/", "/usr/local/bin/", "/usr/local/lib/",
# endif
    "/usr/local/", "/usr/lib/", ""
#endif
} ;

#define PATHTABLE_SIZE (sizeof pathname / sizeof pathname[ 0])


/* does <fname> exist on disk?
 *
 * char *fname;     file to check for existance
 */
boolean fexist( const char *fname) {
    FILE *fp = fopen( fname, "r") ;
    if( fp == NULL)
        return FALSE ;
    else
        fclose( fp) ;

    return TRUE ;
}


/* Look up the existence of a file along the normal or PATH environment
   variable.  Look first in the HOME directory if asked and possible.

   char *fname;     base file name to search for
   boolean hflag;   look in the HOME environment variable first?
 */
char *flook( const char *fname, boolean hflag) {
    static char fspec[ NSTRING] ;   /* full path spec to search */

    int len = sizeof fspec - strlen( fname) - 1 ;
    if( len < 0)
        return NULL ;

#if ENVFUNC
    if( hflag) {
        char *home = getenv( "HOME") ;  /* path to home directory */
        if( home != NULL) {
            if( len > (int) strlen( home) + 1) {
            /* build home dir file spec */
                strcpy( fspec, home) ;
                strcat( fspec, "/") ;
                strcat( fspec, fname) ;

            /* and try it out */
                if( fexist( fspec))
                    return fspec ;
            }
        }
    }
#endif

    /* always try the current directory first */
    if( len >= 0) {
        strcpy( fspec, fname) ;
        if( fexist( fspec))
            return fspec ;
    }

#if ENVFUNC
# if USG | BSD
#  define PATHCHR   ':'
# else
#  define PATHCHR   ';'
# endif

/* get the PATH variable */
    char *path = getenv( "PATH") ;  /* environmental PATH variable */
    if( path != NULL)
        while( *path) {
            int cnt = len ;
        /* build next possible file spec */
            char *sp = fspec ;      /* pointer into path spec */
            while( *path && (*path != PATHCHR)) {
                if( cnt-- > 0)
                    *sp++ = *path ;

                path += 1 ;
            }

            if( cnt >= 0) {
            /* add a terminating dir separator if we need it */
                if (sp != fspec)
                    *sp++ = '/';
                *sp = 0;
                strcat(fspec, fname);

            /* and try it out */
                if( fexist( fspec))
                    return fspec ;
            }

            if( *path == PATHCHR)
                ++path ;
        }
#endif

    /* look it up via the old table method */
    for( unsigned i = 2 ; i < PATHTABLE_SIZE ; i++)
        if( len >= (int) strlen( pathname[ i])) {
            strcpy( fspec, pathname[ i]) ;
            strcat( fspec, fname);

        /* and try it out */
            if( fexist( fspec))
                return fspec ;
        }

    return NULL ;       /* no such luck */
}

/* end of flook.c */
