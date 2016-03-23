/* fileio.c -- implements fileio.h */

#include "fileio.h"

#ifdef	CTRLZ
#undef	CTRLZ
#endif
#define	CTRLZ	0  /* add a ^Z at end of files under MSDOS only    */

/*  FILEIO.C
 *
 * The routines in this file read and write ASCII files from the disk. All of
 * the knowledge about files are here.
 *
 *  modified by Petri Kutvonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "retcode.h"
#include "utf8.h"

char	*fline = NULL ;		/* dynamic return line     */
static int	flen = 0 ;		/* current allocated length of fline */
int		ftype ;
int		fcode ;				/* encoding type FCODE_xxxxx */
int		fpayload ;			/* actual length of fline content */


static FILE		*ffp ;		/* File pointer, all functions. */
static boolean	eofflag ;	/* end-of-file flag */


/*
 * Open a file for reading.
 */
fio_code ffropen( const char *fn)
{
    if ((ffp = fopen(fn, "r")) == NULL)
        return FIOFNF;
    eofflag = FALSE;
    ftype = FTYPE_NONE ;
    fcode = FCODE_ASCII ;
    return FIOSUC;
}

/*
 * Open a file for writing. Return TRUE if all is well, and FALSE on error
 * (cannot create).
 */
fio_code ffwopen( const char *fn)
{
#if     VMS
    int fd;

    if ((fd = creat(fn, 0666, "rfm=var", "rat=cr")) < 0
        || (ffp = fdopen(fd, "w")) == NULL)
#else
    if ((ffp = fopen(fn, "w")) == NULL)
#endif
        return FIOERR;

    return FIOSUC;
}

/*
 * Close a file. Should look at the status in all systems.
 */
fio_code ffclose(void)
{
    /* free this since we do not need it anymore */
    if (fline) {
        free(fline);
        fline = NULL;
    }
    eofflag = FALSE;
    ftype = FTYPE_NONE ;
    fcode = FCODE_ASCII ;

#if MSDOS & CTRLZ
    fputc(26, ffp);     /* add a ^Z at the end of the file */
#endif

#if     V7 | USG | BSD | (MSDOS & (MSC | TURBO))
    if (fclose(ffp) != FALSE)
        return FIOERR;
#else
    fclose(ffp);
#endif
    return FIOSUC;
}

/*
 * Write a line to the already opened file. The "buf" points to the buffer,
 * and the "nbuf" is its length, less the free newline. Return the status.
 * Check only at the newline.
 */
fio_code ffputline( char *buf, int nbuf, int dosflag) {
	fwrite( buf, 1, nbuf, ffp) ;
	
	if( dosflag)
		fputc( '\r', ffp) ;

    fputc( '\n', ffp) ;

    if( ferror( ffp))
        return FIOERR ;

    return FIOSUC ;
}

/*
 * Read a line from a file, and store the bytes in the supplied buffer. The
 * "nbuf" is the length of the buffer. Complain about long lines and lines
 * at the end of the file that don't have a newline present. Check for I/O
 * errors too. Return status.
 */
fio_code ffgetline(void)
{
    int c;      /* current character read */
    int i;      /* current index into fline */
    int lcode = FCODE_ASCII ;	/* line encoding, defaults to ASCII */

    /* if we are at the end...return it */
    if (eofflag)
        return FIOEOF;

    /* dump fline if it ended up too big */
    if (flen > NSTRING) {
        free(fline);
        fline = NULL;
    }

    /* if we don't have an fline, allocate one */
    if (fline == NULL)
        if ((fline = malloc(flen = NSTRING)) == NULL)
            return FIOMEM;

    /* read the line in */
    i = 0;
    while ((c = fgetc(ffp)) != EOF && c != '\r' && c != '\n') {
		fline[i++] = c;
		lcode |= c ;
        /* if it's longer, get more room */
        if (i >= flen) {
		    char *tmpline;  /* temp storage for expanding line */
			    
	        fpayload = i ;
            tmpline = malloc(flen + NSTRING) ;
            if( tmpline == NULL)
    	        return FIOMEM ;

            memcpy( tmpline, fline, flen) ;
            flen += NSTRING;
            free(fline);
            fline = tmpline;
        }
    }

	fpayload = i ;
	lcode &= FCODE_MASK ;
	if( lcode && (fcode != FCODE_MIXED)) {	/* line contains extended chars */
	/* Check if consistent UTF-8 encoding */
		int bytes ;
		int pos = 0 ;
		unicode_t uc ;

		while( (pos < i) && (lcode != FCODE_MIXED)) {
			bytes = utf8_to_unicode( fline, pos, i, &uc) ;
			pos += bytes ;
			if( bytes > 1)		/* Multi byte UTF-8 sequence */
				lcode |= FCODE_UTF_8 ;
			else if( uc > 127)	/* Extended ASCII */
				lcode |= FCODE_EXTND ;
		}

		fcode |= lcode ;
	}

    /* test for any errors that may have occured */
    if (c == EOF) {
        if( ferror( ffp))
            return FIOERR ;

        if (i != 0)
            eofflag = TRUE;
        else
            return FIOEOF;
    } else if( c == '\r') {
        c = fgetc( ffp) ;
        if( c != '\n') {
            ftype |= FTYPE_MAC ;
            ungetc( c, ffp) ;
        } else
	    ftype |= FTYPE_DOS ;
    } else /* c == '\n' */
	ftype |= FTYPE_UNIX ;

    /* terminate the string */
    fline[i] = 0;
    return FIOSUC;
}
