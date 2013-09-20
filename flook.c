/* flook.c -- implements flook.h */
#include "flook.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "defines.h"
#include "fileio.h"


/*	possible names and paths of help files under different OSs	*/
const char *pathname[] = {
#if	MSDOS
	"emacs.rc",
	"emacs.hlp",
	"\\sys\\public\\",
	"\\usr\\bin\\",
	"\\bin\\",
	"\\",
	""
#endif

#if	V7 | BSD | USG
	".emacsrc",
	"emacs.hlp",
#if	PKCODE
	"/usr/global/lib/", "/usr/local/bin/", "/usr/local/lib/",
#endif
	"/usr/local/", "/usr/lib/", ""
#endif

#if	VMS
{
	"emacs.rc", "emacs.hlp", "",
#if	PKCODE
	"sys$login:", "emacs_dir:",
#endif
	"sys$sysdevice:[vmstools]"
#endif
};

#define PATHNAME_SIZE (sizeof pathname / sizeof pathname[ 0])


/*
 * does <fname> exist on disk?
 *
 * char *fname;     file to check for existance
 */
boolean fexist( const char *fname)
{
    FILE *fp;

    /* try to open the file for reading */
    fp = fopen(fname, "r");

    /* if it fails, just return false! */
    if (fp == NULL)
        return FALSE;

    /* otherwise, close it and report true */
    fclose(fp);
    return TRUE;
}

/*
 * Look up the existance of a file along the normal or PATH
 * environment variable. Look first in the HOME directory if
 * asked and possible
 *
 * char *fname;		base file name to search for
 * boolean hflag;		Look in the HOME environment variable first?
 */
char *flook( const char *fname, boolean hflag)
{
	int i;		/* index */
	static char fspec[NSTRING];	/* full path spec to search */

#if	ENVFUNC
	char *path;	/* environmental PATH variable */

	if (hflag) {
		char *home;	/* path to home directory */

		home = getenv("HOME");
		if (home != NULL) {
			/* build home dir file spec */
			strcpy(fspec, home);
			strcat(fspec, "/");
			strcat(fspec, fname);

			/* and try it out */
			if( fexist( fspec))
				return fspec ;
		}
	}
#endif

	/* always try the current directory first */
	strcpy( fspec, fname) ;
	if( fexist( fspec))
		return fspec ;

#if	ENVFUNC
#if	V7 | USG | BSD
#define	PATHCHR	':'
#else
#define	PATHCHR	';'
#endif

	/* get the PATH variable */
	path = getenv("PATH");
	if (path != NULL)
		while (*path) {
			char *sp;	/* pointer into path spec */

			/* build next possible file spec */
			sp = fspec;
			while (*path && (*path != PATHCHR))
				*sp++ = *path++;

			/* add a terminating dir separator if we need it */
			if (sp != fspec)
				*sp++ = '/';
			*sp = 0;
			strcat(fspec, fname);

			/* and try it out */
			if( fexist( fspec))
				return fspec ;

			if (*path == PATHCHR)
				++path;
		}
#endif

	/* look it up via the old table method */
	for( i = 2; i < PATHNAME_SIZE ; i++) {
		strcpy(fspec, pathname[i]);
		strcat(fspec, fname);

		/* and try it out */
		if( fexist( fspec))
			return fspec ;
	}

	return NULL;		/* no such luck */
}

