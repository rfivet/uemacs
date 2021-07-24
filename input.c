/* input.c -- implements input.h */

#include "input.h"

/*  input.c
 *
 *  Various input routines
 *
 *  written by Daniel Lawrence 5/9/86
 *  modified by Petri Kutvonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bind.h"
#include "estruct.h"
#include "bindable.h"
#include "display.h"
#include "exec.h"
#include "isa.h"
#include "names.h"
#include "terminal.h"
#include "utf8.h"
#include "wrapper.h"

#if PKCODE && UNIX
#define COMPLC  1
#else
#define COMPLC  0
#endif

#define NKBDM   256     /* # of strokes, keyboard macro     */
int kbdm[ NKBDM] ;      /* Macro                            */
int *kbdptr ;           /* current position in keyboard buf */
int *kbdend = &kbdm[0] ;    /* ptr to end of the keyboard       */

kbdstate kbdmode = STOP ;   /* current keyboard macro mode  */
int lastkey = 0 ;       /* last keystoke                */
int kbdrep = 0 ;        /* number of repetitions        */

int metac = CONTROL | '[' ;		/* current meta character 	 */
int ctlxc = CONTROL | 'X' ;		/* current control X prefix char */
int reptc = CONTROL | 'U' ;		/* current universal repeat char */
int abortc = CONTROL | 'G' ;		/* current abort command char	 */

const int nlc = CONTROL | 'J' ;		/* end of input char */


void ue_system( const char *cmd) {
    int ret ;

    ret = system( cmd) ;
    if( ret == -1) {
    /* some actual handling needed here */
    }
}

/*
 * Ask a yes or no question in the message line. Return either TRUE, FALSE, or
 * ABORT. The ABORT status is returned if the user bumps out of the question
 * with a ^G. Used any time a confirmation is required.
 */
int mlyesno( const char *prompt)
{
    int	c ;         /* input character */

    for (;;) {
        /* prompt the user */
		mlwrite( "%s (y/n)? ", prompt) ;

        /* get the response */
        c = get1key() ;

        if( c == abortc) /* Bail out! */
            return ABORT;

        if (c == 'y' || c == 'Y')
            return TRUE;

        if (c == 'n' || c == 'N')
            return FALSE;
    }
}

/*
 * newnextarg:
 *  get the next argument
 *
 * char **outbufref ;	buffer to put token into
 * const char *prompt ;	prompt to use if we must be interactive
 * int size ;			size of the buffer
 * int terminator ;		terminating char to be used on interactive fetch
 */
static int newnextarg( char **outbufref, const char *prompt, int size,
															int terminator) {
	int status ;
	char *buf ;

    /* if we are interactive, go get it! */
    if( clexec == FALSE) {
		if( size <= 1) {
			size = term.t_ncol - strlen( prompt) + 1 ;
			if( size < 24)
				size = 24 ;
		}

		buf = malloc( size) ;
		if( buf == NULL)
	    	status = FALSE ;
		else {
			status = getstring( prompt, buf, size, terminator) ;
	        if( TRUE != status) {
	        	free( buf) ;
	        	buf = NULL ;
	        }
	    }
	} else {
		buf = getnewtokval() ;
		status = (buf == NULL) ? FALSE : TRUE ;
	}

	*outbufref = buf ;	
	return status ;
}

/*
 * Write a prompt into the message line, then read back a response. Keep
 * track of the physical position of the cursor. If we are in a keyboard
 * macro throw the prompt away, and return the remembered response. This
 * lets macros run at full speed. The reply is always terminated by a carriage
 * return. Handle erase, kill, and abort keys.
 */

int newmlarg( char **outbufref, const char *prompt, int size) {
	return newnextarg( outbufref, prompt, size, nlc) ;
}

int newmlargt( char **outbufref, const char *prompt, int size) {
	return newnextarg( outbufref, prompt, size, metac) ;
}

/*
 * ectoc:
 *  expanded character to character
 *  collapse the CONTROL and SPEC flags back into an ascii code
 */
int ectoc( int c) {
	if( c & CONTROL)
		c ^= CONTROL | 0x40 ;

	if( c & SPEC)
		c &= 255 ;

	return c ;
}

/*
 * get a command name from the command line. Command completion means
 * that pressing a <SPACE> will attempt to complete an unfinished command
 * name if it is unique.
 */
const name_bind *getname( void) {
    int cpos;   /* current column on screen output */
    const name_bind *ffp;  /* first ptr to entry in name binding table */
    const name_bind *cffp; /* current ptr to entry in name binding table */
    const name_bind *lffp; /* last ptr to entry in name binding table */
    char buf[NSTRING];  /* buffer to hold tentative command name */

    /* starting at the beginning of the string buffer */
    cpos = 0;

    /* if we are executing a command line get the next arg and match it */
    if (clexec) {
		if( TRUE != gettokval( buf, sizeof buf))
            return NULL;
        return fncmatch( buf) ;
    }

    /* build a name string from the keyboard */
    while (TRUE) {
	    int c ;

        c = tgetc();

        /* if we are at the end, just match it */
        if (c == 0x0d) {
            buf[cpos] = 0;

            /* and match it off */
            return fncmatch( buf) ;

        } else if (c == ectoc(abortc)) {    /* Bell, abort */
            ctrlg(FALSE, 0);
            TTflush();
            return NULL;

        } else if (c == 0x7F || c == 0x08) {    /* rubout/erase */
            if (cpos != 0) {
				rubout() ;
                --cpos;
                TTflush();
            }

        } else if (c == 0x15) { /* C-U, kill */
            while (cpos != 0) {
				rubout() ;
                --cpos;
            }

            TTflush();

        } else if (c == ' ' || c == 0x1b || c == 0x09) {
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
        /* attempt a completion */
            buf[ cpos] = 0 ;  /* terminate it for us */
			int buflen = strlen( buf) ;
		/* scan for matches */
            for( ffp = names ; ffp->n_func != NULL ; ffp++) {
                if( strncmp( buf, bind_name( ffp), buflen) == 0) {
                    /* a possible match! More than one? */
                    if( (ffp + 1)->n_func == NULL ||
                        (strncmp( buf, bind_name( ffp + 1), buflen) != 0)) {
                        /* no...we match, print it */
						echos( &bind_name( ffp)[ cpos]) ;
                        TTflush() ;
                        return ffp ;
                    } else {
/* << << << << << << << << << << << << << << << << << */
                        /* try for a partial match against the list */

                        /* first scan down until we no longer match the
						 * current input */
                        for( lffp = ffp + 1 ; (lffp + 1)->n_func != NULL ;
																		lffp++)
                            if( strncmp( buf, bind_name( lffp + 1),
							                                 	buflen) != 0)
                                break ;

                        /* and now, attempt to partial complete the string,
						 * one char at a time */
                        while (TRUE) {
                            /* add the next char in */
                            buf[ cpos] = bind_name( ffp)[ cpos] ;

                            /* scan through the candidates */
                            for( cffp = ffp + 1 ; cffp <= lffp ; cffp++)
                                if( bind_name( cffp)[ cpos] != buf[ cpos])
                                    goto onward ;

                            /* add the character */
                            echoc( buf[ cpos++]) ;
                        }
/* << << << << << << << << << << << << << << << << << */
                    }
                }
            }

            /* no match.....beep and onward */
            TTbeep();
        onward:
            TTflush();
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
        } else {
            if( cpos < NSTRING - 1 && (islower( c) || c == '-')) {
                buf[cpos++] = c;
                echoc( c) ;
				TTflush();
            }
        }
    }
}

/*  tgetc:  Get a key from the terminal driver, resolve any keyboard
        macro action                    */

int tgetc(void)
{
    int c;          /* fetched character */

    /* if we are playing a keyboard macro back, */
    if (kbdmode == PLAY) {

        /* if there is some left... */
        if (kbdptr < kbdend)
            return (int) *kbdptr++;

        /* at the end of last repetition? */
        if (--kbdrep < 1) {
            kbdmode = STOP;
#if VISMAC == 0
            /* force a screen update after all is done */
            update(FALSE);
#endif
        } else {

            /* reset the macro to the begining for the next rep */
            kbdptr = &kbdm[0];
            return (int) *kbdptr++;
        }
    }

    /* fetch a character from the terminal driver */
    c = TTgetc();

    /* record it for $lastkey */
    lastkey = c;

    /* save it if we need to */
    if (kbdmode == RECORD) {
        *kbdptr++ = c;
        kbdend = kbdptr;

        /* don't overrun the buffer */
        if (kbdptr == &kbdm[NKBDM - 1]) {
            kbdmode = STOP;
            TTbeep();
        }
    }

    /* and finally give the char back */
    return c;
}

/*  GET1KEY: Get one keystroke. The only prefixs legal here are the SPEC
    and CONTROL prefixes. */
int get1key( void) {
    int c ;

    /* get a keystroke */
    c = tgetc();

    if( (c >= 0x00 && c <= 0x1F) || c == 0x7F)	/* C0 control -> C-     */
		c ^= CONTROL | 0x40 ;

    return c ;
}

/*  GETCMD: Get a command from the keyboard. Process all applicable
    prefix keys */

static int get1unicode( int *k) {
/* Accept UTF-8 sequence */
	int c = *k ;
	if( c > 0xC1 && c <= 0xF4) {
		char utf[ 4] ;
    	char cc ;

		utf[ 0] = c ;
		utf[ 1] = cc = get1key() ;
		if( (c & 0x20) && ((cc & 0xC0) == 0x80)) { /* at least 3 bytes and a valid encoded char */
			utf[ 2] = cc = get1key() ;
			if( (c & 0x10) && ((cc & 0xC0) == 0x80)) /* at least 4 bytes and a valid encoded char */
				utf[ 3] = get1key() ;
		}

		return utf8_to_unicode( utf, 0, sizeof utf, (unicode_t *) k) ;
	} else
		return 1 ;
}

int getcmd(void)
{
    int c;          /* fetched keystroke */
#if VT220
    int d;          /* second character P.K. */
    int cmask = 0;
#endif
    /* get initial character */
    c = get1key();

#if VT220
      proc_metac:
#endif
    if (c == 128+27)        /* CSI */
        goto handle_CSI;
    /* process META prefix */
    if (c == (CONTROL | '[')) {
        c = get1key();
#if VT220
        if (c == '[' || c == 'O') { /* CSI P.K. */
handle_CSI:
            c = get1key();
            if (c >= 'A' && c <= 'D')
                return SPEC | c | cmask;
            if (c >= 'E' && c <= 'z' && c != 'i' && c != 'c')
                return SPEC | c | cmask;
            d = get1key();
            if (d == '~')   /* ESC [ n ~   P.K. */
                return SPEC | c | cmask;
            switch (c) {    /* ESC [ n n ~ P.K. */
            case '1':
                c = d + 32;
                break;
            case '2':
                c = d + 48;
                break;
            case '3':
                c = d + 64;
                break;
            default:
                c = '?';
                break;
            }
            if (d != '~')   /* eat tilde P.K. */
                get1key();
            if (c == 'i') { /* DO key    P.K. */
                c = ctlxc;
                goto proc_ctlxc;
            } else if (c == 'c')    /* ESC key   P.K. */
                c = get1key();
            else
                return SPEC | c | cmask;
        }
#endif
#if VT220
        if (c == (CONTROL | '[')) {
            cmask = META;
            goto proc_metac;
        }
#endif
        if( islower( c)) /* Force to upper */
            c = flipcase( c) ;
        else if( c >= 0x00 && c <= 0x1F) /* control key */
            c = CONTROL | (c + '@');
        return META | c;
    }
#if PKCODE
    else if (c == metac) {
        c = get1key();
#if VT220
        if (c == (CONTROL | '[')) {
            cmask = META;
            goto proc_metac;
        }
#endif
        if( islower( c)) /* Force to upper */
            c = flipcase( c) ;
        else if( c >= 0x00 && c <= 0x1F) /* control key */
            c = CONTROL | (c + '@');
        return META | c;
    }
#endif


#if VT220
      proc_ctlxc:
#endif
    /* process CTLX prefix */
    if (c == ctlxc) {
        c = get1key();
#if VT220
        if (c == (CONTROL | '[')) {
            cmask = CTLX;
            goto proc_metac;
        }
#endif
        if (c >= 'a' && c <= 'z')   /* Force to upper */
            c -= 0x20;
        if (c >= 0x00 && c <= 0x1F) /* control key */
            c = CONTROL | (c + '@');
        return CTLX | c;
    }

#ifdef CYGWIN
	get1unicode( &c) ;
#endif
	
    /* otherwise, just return it */
    return c;
}

/*  A more generalized prompt/reply function allowing the caller
    to specify the proper terminator. If the terminator is not
    a return ('\n') it will echo as "<NL>"
*/

static void echov( int c) {
/* verbose echo of a character */
	if( c == '\n')		/* put out <NL> for <ret> */
		echos( "<NL>") ;
	else {
		if( c < ' ' || c == 0x7F) {
			echoc( '^') ;
			c ^= 0x40 ;
		}

		echoc( c) ;
	}
}

static void rubc( char c) {
	rubout() ;
	if( (c >= 0 && c < ' ') || c == 0x7F) {
	/* ^x range */
		rubout() ;
		if( c == '\n') {
			rubout() ;
			rubout() ;
		}
	}
}

int getstring( const char *prompt, char *buf, int nbuf, int eolchar)
{
    int cpos;   /* current character position in string */
    int c;
    boolean quote_f ;    /* are we quoting the next char? */
	int retval ;		/* TRUE, FALSE, ABORT */
#if COMPLC
	boolean file_f ;
    int ocpos, nskip = 0, didtry = 0;
#if     MSDOS
    struct ffblk ffblk;
    char *fcp;
#endif
#if UNIX
    static char tmp[] = "/tmp/meXXXXXX";
    FILE *tmpf = NULL;
#endif
/*	Look for "Find file: ", "View file: ", "Insert file: ", "Write file: ",
**	"Read file: ", "Execute file: " */
	file_f = NULL != strstr( prompt, " file: ") ;
#endif

    cpos = 0;
    quote_f = FALSE;

    /* prompt the user for the input string */
    mlwrite( "%s", prompt);

    for (;;) {
#if COMPLC
        if (!didtry)
            nskip = -1;
        didtry = 0;
#endif
	/* get a character from the user */
        c = get1key();

	/* Quoting? Store as it is */
		if( quote_f == TRUE) {
			quote_f = FALSE ;
			if( cpos < nbuf - 1) {
				c = ectoc( c) ;
				buf[ cpos++] = c ;
				echov( c) ;
				TTflush() ;
			}

			continue ;
		}

	/* If it is a <ret>, change it to a <NL> */
        if( c == (CONTROL | 'M'))
            c = CONTROL | 0x40 | '\n' ;

        if( c == eolchar) {
		/* if they hit the line terminator, wrap it up */
            buf[ cpos] = 0 ;

		/* clear the message line */
            mlwrite("");

		/* if we default the buffer, return FALSE */
			retval = cpos != 0 ;
			break ;
        } else if( c == abortc) {
		/* Abort the input? */
            ctrlg( FALSE, 0) ;
			retval = ABORT ;
			break ;
		}

	/* change from command form back to character form */
        c = ectoc( c) ;

        if( c == 0x7F || c == 0x08) {
		/* rubout/erase */
            if (cpos != 0) {
				rubc( buf[ --cpos]) ;
				cpos -= utf8_revdelta( (unsigned char *) &buf[ cpos], cpos) ;
                TTflush();
            }
        } else if( c == 0x15) {
        /* C-U, kill */
			mlwrite( "%s", prompt) ;
			cpos = 0 ;
#if COMPLC
        } else if( (c == 0x09 || c == ' ') && file_f) {
        /* TAB, complete file name */
            char ffbuf[255];
#if MSDOS
            char sffbuf[128];
            int lsav = -1;
#endif
            int n, iswild = 0;

            didtry = 1;
            ocpos = cpos;
			mlwrite( "%s", prompt) ;
			while( cpos != 0) {
				c = buf[ --cpos] ;
                if( c == '*' || c == '?') {
                    iswild = 1 ;
					cpos = 0 ;
					break ;
				}
			}

            if (nskip < 0) {
                buf[ocpos] = 0;
#if UNIX
                if (tmpf != NULL) {
                    fclose(tmpf);
                    tmpf = NULL ;
                    unlink( tmp) ;
                }

                strcpy( tmp, "/tmp/meXXXXXX") ;
                xmkstemp( tmp) ;
                if( strlen( buf) < sizeof ffbuf - 26 - 1)
					sprintf( ffbuf, "echo %s%s >%s 2>&1", buf,
													!iswild ? "*" : "", tmp) ;
				else
					sprintf( ffbuf, "echo ERROR >%s 2>&1", tmp) ;

                ue_system( ffbuf) ;
                tmpf = fopen(tmp, "r");
#endif
#if MSDOS
                strcpy(sffbuf, buf);
                if (!iswild)
                    strcat(sffbuf, "*.*");
#endif
                nskip = 0;
            }
#if UNIX
            c = ' ';
            for (n = nskip; n > 0; n--)
                while ((c = getc(tmpf)) != EOF
                       && c != ' ');
#endif
#if MSDOS
            if (nskip == 0) {
                strcpy(ffbuf, sffbuf);
                c = findfirst(ffbuf, &ffblk,
                          FA_DIREC) ? '*' : ' ';
            } else if (nskip > 0)
                c = findnext(&ffblk) ? 0 : ' ';
#endif
            nskip++;

            if (c != ' ') {
                TTbeep();
                nskip = 0;
            }
#if UNIX
            while ((c = getc(tmpf)) != EOF && c != '\n'
                   && c != ' ' && c != '*')
#endif
#if MSDOS
                if (c == '*')
                    fcp = sffbuf;
                else {
                    strncpy(buf, sffbuf, lsav + 1);
                    cpos = lsav + 1;
                    fcp = ffblk.ff_name;
                }
            while (c != 0 && (c = *fcp++) != 0 && c != '*')
#endif
            {
                if (cpos < nbuf - 1)
                    buf[cpos++] = c;
            }
#if UNIX
            if (c == '*')
                TTbeep();
#endif

            for( n = 0 ; n < cpos ; ) {
				n += utf8_to_unicode( buf, n, nbuf, (unicode_t *) &c) ;
				echov( c) ;
            }

            TTflush() ;
#if UNIX
            rewind(tmpf);
#endif
#endif

        } else if( c == 0x11 || c == 0x16)
		/* ^Q or ^V */
			quote_f = TRUE ;
        else {
		/* store as it is */
			int n ;

			n = get1unicode( &c) ;	/* fetch multiple bytes */
            if( cpos + n < nbuf) {
				cpos += unicode_to_utf8( c, &buf[ cpos]) ;
				echov( c) ;
				TTflush() ;
            }
		}
    }

	TTflush() ;
	if( tmpf != NULL) {
		fclose( tmpf) ;
		unlink( tmp) ;
	}

	return retval ;
}
