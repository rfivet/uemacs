/* exec.c -- implements exec.h */
#include "exec.h"

/* This file is for bindable functions dealing with execution of commands,
   command lines, buffers, files and startup files.

   written 1986 by Daniel Lawrence
   modified by Petri Kutvonen
 */

#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "bind.h"
#include "defines.h"	/* malloc/allocate, free/release */
#include "eval.h"
#include "file.h"
#include "flook.h"
#include "input.h"
#include "line.h"
#include "mlout.h"
#include "random.h"
#include "util.h"
#include "window.h"

static char *execstr = NULL ;	/* pointer to string to execute */
boolean clexec = FALSE ;    	/* command line execution flag  */

/*  Directive definitions   */

#define DIF     	0
#define DELSE       1
#define DENDIF      2
#define DGOTO       3
#define DRETURN     4
#define DENDM       5
#define DWHILE      6
#define DENDWHILE   7
#define DBREAK      8
#define DFORCE      9


/* The !WHILE directive in the execution language needs to
 * stack references to pending whiles. These are stored linked
 * to each currently open procedure via a linked list of
 * the following structure.
*/
typedef struct while_block {
    line_p w_begin ;				/* ptr to !while statement */
    line_p w_end ;					/* ptr to the !endwhile statement */
    int w_type ;					/* block type */
    struct while_block *w_next ;	/* next while */
} *while_p ;

#define BTWHILE     1
#define BTBREAK     2

/* directive name table: holds the names of all the directives....  */

static const char *dname[] = {
    "if", 	"else", 	"endif",	"goto",		"return",
	"endm", "while",	"endwhile", "break",	"force"
} ;

#define NUMDIRS     ARRAY_SIZE( dname)

static char golabel[ NSTRING] = "" ;	/* current line to go to */
static buffer_p bstore = NULL ;			/* buffer to store macro text to */
static int storing_f = FALSE ;			/* storing text to macro flag */

static int dobuf( buffer_p bp) ;
static int macarg( char *tok, int toksz) ;

/* Execute a named command even if it is not bound. */
BINDABLE( namedcmd) {
/* prompt the user to type a named command */
    mloutstr( "execute-named-cmd: ");

/* and now get the function name to execute */
	nbind_p nbp = getname() ;
	if( nbp == NULL)	/* abort */
		return ABORT ;

    fnp_t kfunc = nbp->n_func ;
    if( kfunc == NULL)
		return mloutfail( "(No such function)") ;

	if( (bind_tag( nbp) & 1) && (curbp->b_mode & MDVIEW))
		return rdonly() ;

    /* and then execute the command */
    return kfunc( f, n) ;
}

static int docmd( char *cline) ;

/* execcmd:
 *  Execute a command line command to be typed in
 *  by the user
 *
 * int f, n;        default Flag and Numeric argument
 */
BINDABLE( execcmd) {
    char *cmdstr ;	/* string holding command to execute */

/* get the line wanted */
    int status = newmlarg( &cmdstr, "execute-command-line: ", 0) ;
    if( status != TRUE)
        return status ;

    while( status == TRUE && n-- > 0)
        status = docmd( cmdstr) ;

    free( cmdstr) ;
    return status ;
}

/* docmd:
 *  take a passed string as a command line and translate
 *  it to be executed as a command. This function will be
 *  used by execute-command-line and by all source and
 *  startup files. Lastflag/thisflag is also updated.
 *
 *  format of the command line is:
 *
 *      {# arg} <command-name> {<argument string(s)>}
 *
 * char *cline;     command line to execute
 */
static int docmd( char *cline) {
    char tkn[ NSTRING] ;	/* next token off of command line */

    char *oldestr = execstr ;	/* save last ptr to string to execute */
    execstr = cline ;   		/* and set this one as current */

    /* first set up the default command values */
    int f = FALSE ;
    int n = 1 ;
    lastflag = thisflag ;
    thisflag = 0 ;

    int status = macarg( tkn, sizeof tkn) ;
    if( status != TRUE) {   /* and grab the first token */
        execstr = oldestr;
        return status;
    }

    /* process leadin argument */
    if( !is_it_cmd( tkn)) {
        f = TRUE;
/* macarg already includes a getval, skip for now
		mystrscpy( tkn, getval( tkn), sizeof tkn) ;
*/
        n = atoi(tkn);

        /* and now get the command to execute */
        status = macarg( tkn, sizeof tkn) ;
        if( status != TRUE) {
            execstr = oldestr ;
            return status ;
        }
    }

    /* and match the token to see if it exists */
	nbind_p nbp = fncmatch( tkn) ;
    fnp_t fnc = nbp->n_func ;
    if( fnc == NULL) {
        execstr = oldestr ;
        return mloutfail( "(No such Function)") ;
    }

	if( (bind_tag( nbp) & 1) && (curbp->b_mode & MDVIEW))
		status = rdonly() ;
	else {
	    /* save the arguments and go execute the command */
    	boolean oldcle = clexec ;    /* save old clexec flag */
    	clexec = TRUE ;      		/* in cline execution */
	    status = fnc( f, n) ;		/* call the function */
	    clexec = oldcle ;    		/* restore clexec flag */
    	execstr = oldestr ;
	}

    cmdstatus = status ; /* save the status */
    return status ;
}

/*
 * new token:
 *  chop a token off a string
 *  return a pointer past the token
 *
 * char *src		in, source string
 * char **tokref	out, destination of newly allocated token string
 */
static char *newtoken( char *src, char **tokref) {
	char *tok = malloc( NSTRING) ;
	int size = (tok == NULL) ? 0 : NSTRING ;
    int idx = 0 ;	/* insertion point into token string */

    /* first scan past any whitespace in the source string */
    while( *src == ' ' || *src == '\t')
        ++src ;

    /* scan through the source string */
    boolean quotef = FALSE ;	/* is the current string quoted? */
    while( *src) {
	    char c ;	/* temporary character */

        /* process special characters */
        if( *src == '~') {
            ++src;
            if (*src == 0)
                break;
            switch (*src++) {
            case 'r':
                c = 13;
                break;
            case 'n':
                c = 10;
                break;
            case 't':
                c = 9;
                break;
            case 'b':
                c = 8;
                break;
            case 'f':
                c = 12;
                break;
            default:
                c = *(src - 1);
            }
        } else {
            /* check for the end of the token */
            if (quotef) {
                if (*src == '"')
                    break;
            } else {
                if (*src == ' ' || *src == '\t')
                    break;
            }

            /* set quote mode if quote found */
            if (*src == '"')
                quotef = TRUE;

            /* record the character */
            c = *src++;
        }

		if( idx < size - 1)
			tok[ idx++] = c ;
		else if( size > 1) {
			char *tmptok ;
			
			tmptok = malloc( size + 32) ;
			if( tmptok == NULL)
				size = 0 ;
			else {
				memcpy( tmptok, tok, idx) ;
				free( tok) ;
				tok = tmptok ;
				size += 32 ;
				tok[ idx++] = c ;
			}
		}
    }

    /* terminate the token and exit */
    if (*src)
        ++src;

    if( tok != NULL)
        tok[ idx] = 0 ;

    *tokref = tok ;
    return src;
}

static char *token( char *srcstr, char *tok, int maxtoksize) {
	char *newtok ;

	srcstr = newtoken( srcstr, &newtok) ;
	if( newtok == NULL)
		tok[ 0] = 0 ;
	else {
		mystrscpy( tok, newtok, maxtoksize) ;
		free( newtok) ;
	}

	return srcstr ;
}

void gettoken( char *tok, int maxtoksize) {
	execstr = token( execstr, tok, maxtoksize) ;
}

static char *getnewtoken( void) {
	char *tok ;

	execstr = newtoken( execstr, &tok) ;
	return tok ;
}

boolean gettokval( char *tok, int size) {
	char *tmpbuf ;

    /* grab token and advance past */
	tmpbuf = getnewtoken() ;
	if( tmpbuf == NULL)
		return FALSE ;

    /* evaluate it */
    mystrscpy( tok, getval( tmpbuf), size) ;
    free( tmpbuf) ;
    return TRUE ;
}

char *getnewtokval( void) {
	char *tmpbuf ;
	char *valbuf ;

    /* grab token and advance past */
	tmpbuf = getnewtoken() ;
	if( tmpbuf == NULL)
		return NULL ;

    /* evaluate it */
    const char *tmpval = getval( tmpbuf) ;
    valbuf = malloc( strlen( tmpval) + 1 ) ;
    if( valbuf != NULL)
		strcpy( valbuf, tmpval) ;

    free( tmpbuf) ;
    return valbuf ;
}

/*
 * get a macro line argument
 *
 * char *tok;       buffer to place argument
 */
static int macarg( char *tok, int toksz) {
	int status ;
	boolean savcle ;	/* buffer to store original clexec */

	savcle = clexec ;	/* save execution mode */
	clexec = TRUE ;		/* get the argument */
	status = gettokval( tok, toksz) ;
	clexec = savcle ;	/* restore execution mode */
	return status ;
}


/* storemac:
 *  Set up a macro buffer and start recording all command lines until !endm
 *
 * int f;       default flag
 * int n;       macro number to use
 */
static char macbufname[] = "*Macro xx*" ;
#define MACDIGITPOS 7

static boolean setstore( char *bufname) {
/* set up the new macro buffer */
    bstore = bfind( bufname, TRUE, BFINVS) ;
    if( bstore == NULL) {
		storing_f = FALSE ;	/* should be already the case as we are executing */
        return mloutfail( "Can not create macro") ;
	}

/* and make sure it is empty */
    bclear( bstore) ;

/* start recording */
    storing_f = TRUE ;
    return TRUE ;
}


BBINDABLE( storemac) {
/* must have a numeric argument to this function */
    if( f == FALSE)
        return mloutfail( "No macro number specified");

/* range check the macro number */
    if( n < 1 || n > 40)
        return mloutfail( "Macro number out of range") ;

/* construct the macro buffer name */
    macbufname[ MACDIGITPOS]     = '0' + (n / 10) ;
    macbufname[ MACDIGITPOS + 1] = '0' + (n % 10) ;

	return setstore( macbufname) ;
}


/*  exec -- execute a buffer
**	common to execute buffer, procedure and macro
*/
static int exec( int n, char *bufname, char *errstr) {
    /* find the pointer to that buffer */
    buffer_p bp = bfind( bufname, FALSE, 0) ;
    if( bp == NULL) {
        mloutfmt( "No such %s", errstr) ;
		return FALSE ;
	}

    /* and now execute it as asked */
    int status = TRUE ;
    while( status == TRUE && n-- > 0)
        status = dobuf( bp) ;

    return status ;
}


/* storeproc:
 *  Set up a procedure buffer and start recording all command lines until !endm
 *
 * int f;       default flag
 * int n;       macro number to use
 */
BINDABLE( storeproc) {
    bname_t bname ;		/* name of buffer to use */
    char *name ;

    /* a numeric argument means it is a numbered macro */
    if( f == TRUE)
        return storemac( f, n) ;

    /* get the name of the procedure */
    int status = newmlarg( &name, "Procedure name: ", sizeof bname - 2) ;
    if( status != TRUE)
        return status ;

    /* construct the macro buffer name */
    bname[ 0] = '*';
    mystrscpy( &bname[ 1], name, sizeof bname - 2) ;
    strcat( bname, "*") ;
	free( name) ;

	return setstore( bname) ;
}


/* execproc:
 *  Execute a procedure
 *
 * int f, n;        default flag and numeric arg
 */
BINDABLE( execproc) {
    bname_t bufn ;	/* name of buffer to execute */
    char *name ;

    /* find out what buffer the user wants to execute */
    int status = newmlarg( &name, "execute-procedure: ", sizeof bufn - 2) ;
    if( status != TRUE)
        return status ;

    /* construct the buffer name */
    bufn[ 0] = '*' ;
    mystrscpy( &bufn[ 1], name, sizeof bufn - 2) ;
    strcat( bufn, "*") ;
    free( name) ;

	return exec( n, bufn, "procedure") ;
}


/* execbuf:
 *  Execute the contents of a buffer of commands
 *
 * int f, n;        default flag and numeric arg
 */
BINDABLE( execbuf) {
    char *bufn ;		/* name of buffer to execute */

/* find out what buffer the user wants to execute */
    int status = newmlarg( &bufn, "execute-buffer: ", sizeof( bname_t)) ;
    if( status != TRUE)
        return status ;

    status = exec( n, bufn, "buffer") ;
    free( bufn) ;
    return status ;
}


/* free a list of while block pointers
 *
 * while_p wp;      head of structure to free
 */
static void freewhile( while_p wp) {
	while( wp != NULL) {
		while_p next = wp->w_next ;
		free( wp) ;
		wp = next ;
	}
}


static boolean storeline( char *eline, int linlen) {
/* allocate the space for the line */
   	line_p mp = lalloc( linlen) ;
   	if( mp == NULL)
   		return mloutfail( "Out of memory while storing macro") ;

/* copy the text into the new line */
	memcpy( mp->l_text, eline, linlen) ; /* lalloc has set lp->l_used */

/* attach the line to the end of the buffer */
    bstore->b_linep->l_bp->l_fp = mp ;
    mp->l_bp = bstore->b_linep->l_bp ;
    bstore->b_linep->l_bp = mp ;
    mp->l_fp = bstore->b_linep ;
	return TRUE ;
}


/* dobuf:
 *  execute the contents of the buffer pointed to by the passed BP
 *
 *  Directives start with a "!" and include:
 *
 *  !endm       End a macro
 *  !if (cond)  conditional execution
 *  !else
 *  !endif
 *  !return     Return (terminating current macro)
 *  !goto <label>   Jump to a label in the current macro
 *  !force      Force macro to continue...even if command fails
 *  !while (cond)   Execute a loop if the condition is true
 *  !endwhile
 *
 *  Line Labels begin with a "*" or ':' as the first nonblank char, like:
 *
 *  *LBL01 or :LBL01
 *
 * buffer_p bp;       buffer to execute
 */
static int dobuf( buffer_p bp) {
    while_p whtemp ;		/* temporary ptr to a struct while_block */
    char *eline ;       	/* text of line to execute */
    char tkn[ NSTRING] ;	/* buffer to evaluate an expression in */

/* clear IF level flags/while ptr */
    while_p whlist = NULL ;		/* ptr to !WHILE list */
    while_p scanner = NULL ;	/* ptr during scan */

/* range of lines with labels, initially no lines in range */
	line_p firstlbl = NULL ;
	line_p eolbl = firstlbl ;

/* parse the buffer to execute, building WHILE header blocks */
	storing_f = FALSE ;
    const line_p hlp = bp->b_linep ;	/* pointer to line header */
    for( line_p lp = hlp->l_fp ; lp != hlp ; lp = lp->l_fp) {
    /* scan the current line */
        eline = lp->l_text ;
		char const *eol = &eline[ lp->l_used] ;
		for( ; eline < eol ; eline++)
			if( *eline != ' ' && *eline != '\t')
				break ;

	/* empty blank and comment lines */
		if( eline == eol || *eline == '#' || *eline == ';') {
			if( bp->b_nwnd != 0)
				lp->l_used = 0 ;
			else {
			/* delete line if buffer is not displayed */
				line_p curlp = lp ;
				lp = lp->l_bp ;
				lp->l_fp = curlp->l_fp ;
				curlp->l_fp->l_bp = lp ;
				free( curlp) ;
			}
	
			continue ;
		}

	/* remove leading spaces */
		if( eline != lp->l_text) {
			int size = lp->l_used = eol - eline ;
			if( size)
				memcpy( lp->l_text, eline, size) ;

			eline = lp->l_text ;
			eol = &lp->l_text[ size] ;
		}

	/* handle storing */
		if( storing_f) {
			if( !strncmp( eline, "!endm", 5)) {
				bstore = NULL ;
				storing_f = FALSE ;
			} else if( !storeline( lp->l_text, lp->l_used))
				goto failexit ;

			lp->l_used = 0 ;
			continue ;			
		}

	/* process labels, update the range of lines containing labels */
		if( *eline == ':' || *eline == '*') {
			if( firstlbl == NULL)
				firstlbl = lp ;

			eolbl = lp->l_fp ;
			continue ;
		}

	/* process directives, skip others */
		if( *eline != '!')
			continue ;

		if( !strncmp( eline, "!store", 6)) {
			if( lp->l_used < lp->l_size) {
				eline[ lp->l_used] = 0 ;
				execstr = &eline[ 6] ;
				gettoken( &tkn[ 1], sizeof tkn - 1) ;
				char c = tkn[ 1] ;
				if( c >= '1' && c <= '9') { /* number >= 1 */
					if( !storemac( TRUE, atoi( &tkn[ 1])))
						goto failexit ;
				} else {	/* whatever */
					*tkn = '*' ;
					strcat( tkn, "*") ;
					if( !setstore( tkn))
						goto failexit ;
				}
			}

			lp->l_used = 0 ;
			continue ;
		} else if( !strncmp( eline, "!while", 6)) {
    	/* if it is a while directive, make a block... */
	    	whtemp = malloc( sizeof *whtemp) ;
        	if( whtemp == NULL) {
        	noram:
				mloutstr( "%%Out of memory during while scan") ;
				goto failexit ;
        	}
			
        	whtemp->w_begin = lp ;
        	whtemp->w_type = BTWHILE ;
        	whtemp->w_next = scanner ;
        	scanner = whtemp ;
		} else if( !strncmp( eline, "!break", 6)) {
		/* if is a BREAK directive, make a block... */
			if( scanner == NULL) {
    			mloutstr( "%%!BREAK outside of any !WHILE loop") ;
        		goto failexit ;
    		}

        	whtemp = malloc( sizeof *whtemp) ;
	        if( whtemp == NULL)
    	        goto noram ;

        	whtemp->w_begin = lp ;
			whtemp->w_type = BTBREAK ;
			whtemp->w_next = scanner ;
			scanner = whtemp ;
		} else if( !strncmp( eline, "!endwhile", 9)) {
		/* if it is an endwhile directive, record the spot... */
			if( scanner == NULL) {
				mloutfmt( "%%!ENDWHILE with no preceding !WHILE in '%s'",
																bp->b_bname) ;
				goto failexit ;
			}
	
        /* move top records from the scanner list to the whlist until we
           have moved all BREAK records and one WHILE record */
        	do {
            	scanner->w_end = lp ;
            	whtemp = whlist ;
            	whlist = scanner ;
            	scanner = scanner->w_next ;
            	whlist->w_next = whtemp ;
            } while( whlist->w_type == BTBREAK) ;
        }
    }

/* check consistency after parsing */
	if( storing_f) {
	/* missing !endm */
		mloutstr( "!store without !endm") ;
		goto failexit ;
	}
	
    if( scanner != NULL) {
    /* while and endwhile should match! */
		mloutfmt( "%%!WHILE with no matching !ENDWHILE in '%s'", bp->b_bname) ;
	failexit:
		freewhile( scanner) ;
   	    freewhile( whlist) ;
   	    return FALSE ;
    }

/* execute the parsed buffer */
    /* let the first command inherit the flags from the last one.. */
    thisflag = lastflag;

    /* starting at the beginning of the buffer */
    char *einit = NULL ;	/* initial value of eline */
	int status = TRUE ;		/* status of command execution */
	boolean done = FALSE ;
    int execlevel = 0 ;		/* execution IF level */
	line_p lp = hlp->l_fp ;
    for( ; lp != hlp ; lp = lp->l_fp) {
		if( einit) {
			free( einit) ;
			einit = NULL ;
		}

    /* allocate eline and copy macro line to it */
        int linlen = lp->l_used ;
		if( linlen == 0)
			continue ;

        einit = eline = malloc( linlen + 1) ;
        if( eline == NULL) {
            status = mloutfail( "%%Out of Memory during macro execution") ;
			break ;
        }

		memcpy( eline, lp->l_text, linlen) ;
		eline[ linlen] = 0 ;

#if DEBUGM
        /* if $debug == TRUE, every line to execute
           gets echoed and a key needs to be pressed to continue
           ^G will abort the command */

		if( macbug) {
        /* debug macro name, if levels and lastly the line */
			int c = mdbugout( "<<<%s:%d:%s>>>", bp->b_bname, execlevel, eline) ;
        	if( c == abortc) {
                status = FALSE ;
				break ;
        	} else if( c == metac) {
        		macbug = FALSE ;
        	}
        }
#endif

        /* if macro store is on, just salt this away */
        if( storing_f) {
			if( !strncmp( eline, "!endm", 5)) {
                storing_f = FALSE ;
                bstore = NULL ;
			} else {
				status = storeline( eline, strlen( eline)) ;
				if( status != TRUE)
					break ;
			}

			lp->l_used = 0 ;
            continue ;
        }

        /* skip labels */
        if( *eline == '*' || *eline == ':')
            continue ;

    /* Parse directives here.... */
        if( *eline == '!') {
		    unsigned dirnum ;			/* directive index */

        /* Find out which directive this is */
            ++eline ;
            for( dirnum = 0 ; dirnum < NUMDIRS ; dirnum++)
                if( !strncmp( eline, dname[ dirnum], strlen( dname[ dirnum])))
                    break ;

        /* and bitch if it's illegal */
            if( dirnum == NUMDIRS) {
                status = mloutfail( "%%Unknown Directive") ;
				break ;
            }

			--eline ;	/* restore the original eline.... */

		/* now, execute directives */
            /* skip past the directive */
            while( *eline && *eline != ' ' && *eline != '\t')
                ++eline ;

            while( *eline && (*eline == ' ' || *eline == '\t'))
                ++eline ;

            execstr = eline ;
            switch( dirnum) {
			case DENDM:
				if( execlevel == 0)
					status = mloutfail( "!endm out of context") ;

				break ;

            case DIF:   	/* IF directive */
            /* grab the value of the logical exp */
                if( execlevel == 0) {
                    status = macarg( tkn, sizeof tkn) ;
                    if( status == TRUE && stol( tkn) == FALSE)
                        ++execlevel ;
                } else
                    ++execlevel ;

                break ;

            case DWHILE:	/* WHILE directive */
            /* grab the value of the logical exp */
                if( execlevel == 0) {
                    status = macarg( tkn, sizeof tkn) ;
                    if( status != TRUE) {
						break ;
                    } else if( stol( tkn) == TRUE)
						break ;
                }
            /* drop down and act just like !BREAK */

				/* fallthrough */
            case DBREAK:    /* BREAK directive */
                if( dirnum == DBREAK && execlevel)
                    continue ;

                /* jump down to the endwhile */
                /* find the right while loop */
                for( whtemp = whlist ; whtemp ; whtemp = whtemp->w_next)
                    if( whtemp->w_begin == lp)
                        break ;

                if( whtemp == NULL)
                    status = mloutfail( "%%Internal While loop error") ;
                else
                /* reset the line pointer back.. */
	                lp = whtemp->w_end ;

				break ;
            case DELSE:		/* ELSE directive */
                if( execlevel == 1)
                    --execlevel ;
                else if( execlevel == 0)
                    ++execlevel ;

				break ;
            case DENDIF:	/* ENDIF directive */
                if( execlevel)
                    --execlevel;

				break ;
            case DGOTO:		/* GOTO directive */
            /* .....only if we are currently executing */
                if( execlevel != 0)
					break ;

				if( firstlbl != NULL) {
				    line_p glp ;   /* line to goto */

            	    /* grab label to jump to */
                    eline = token( eline, golabel, sizeof golabel) ;
                    linlen = strlen( golabel) ;
                    for( glp = firstlbl ; glp != eolbl ; glp = glp->l_fp) {
						char c = *glp->l_text ;
                        if(	(c == '*' || c == ':')
						&&	!strncmp( &glp->l_text[ 1], golabel, linlen)) {
							lp = glp ;
                            break ;
                        }
                    }

					if( glp == eolbl)
						goto nolabel ;
				} else {
				nolabel:
       	            status =  mloutfail( "%%No such label") ;
				}

				break ;
            case DRETURN:   /* RETURN directive */
                if( execlevel == 0)
                    done = TRUE ;

				break ;
            case DENDWHILE: /* ENDWHILE directive */
                if( execlevel)
                    --execlevel ;
                else {
                /* find the right while loop */
                    for( whtemp = whlist ; whtemp ; whtemp = whtemp->w_next)
                        if( whtemp->w_type == BTWHILE
                        &&	whtemp->w_end == lp)
                            break ;

                    if( whtemp == NULL)
                        status = mloutfail( "%%Internal While loop error") ;
                    else
    	            /* reset the line pointer back.. */
	                    lp = whtemp->w_begin->l_bp ;
                }

				break ;
            case DFORCE:		/* FORCE directive */
				if( execlevel == 0)
					docmd( eline) ;		/* execute ignoring returned status */
            }
		} else if( execlevel == 0)
			status = docmd( eline) ;	/* execute the statement */

   	    if( done || status != TRUE)
			break ;
	}

/* check for a command error */
	if( status != TRUE) {
    /* look if buffer is showing */
       	for( window_p wp = wheadp ; wp != NULL ; wp = wp->w_wndp) {
           	if( wp->w_bufp == bp) {
            /* and point it */
                wp->w_dotp = lp ;
   	            wp->w_doto = 0 ;
       	        wp->w_flag |= WFHARD ;
            }
        }

    /* in any case set the buffer . */
        bp->b_dotp = lp ;
        bp->b_doto = 0 ;
    }

    freewhile( whlist) ;
    if( einit)
		free( einit) ;

    return status ;
}


/* execute a series of commands in a file
 *
 * int f, n;        default flag and numeric arg to pass on to file
 */
BINDABLE( execfile) {
    char *fname ;	/* name of file to execute */

    int status = newmlarg( &fname, "execute-file: ", 0) ;
    if( status != TRUE)
        return status ;

/* look up the path for the file */
    char *fspec = flook( fname, FALSE) ;    /* used to be TRUE, P.K. */
    free( fname) ;

/* if it isn't around */
    if( fspec == NULL)
        return FALSE ;

/* otherwise, execute it */
    while( status == TRUE && n-- > 0)
        status = dofile( fspec) ;

    return status ;
}

/* dofile:
 *  yank a file into a buffer and execute it
 *  if there are no errors, delete the buffer on exit
 *
 * char *fname;     file name to execute
 */
int dofile( const char *fname) {
    bname_t bname ;				/* name of buffer */

    makename( bname, fname) ;	/* derive the name of the buffer */
    unqname( bname) ;			/* make sure we don't stomp things */
    buffer_p bp = bfind( bname, TRUE, 0) ;	   /* get the needed buffer */
    if( bp == NULL)
        return FALSE ;

    bp->b_mode = MDVIEW ;		/* mark the buffer as read only */
    buffer_p cb = curbp ;		/* save the old buffer */
    curbp = bp ;				/* make this one current */
/* and try to read in the file to execute */
    int status = readin( fname, FALSE) ;
    curbp = cb ;				/* restore the current buffer */
	if( status == TRUE) {
	/* go execute it! */
		status = dobuf( bp) ;
		if( status == TRUE && bp->b_nwnd == 0)
	    /* if not displayed, remove the now unneeded buffer and exit */
			zotbuf( bp) ;
	}

	return status ;
}

/* cbuf:
 *  Execute the contents of a numbered buffer
 *
 * int f, n;        default flag and numeric arg
 * int bufnum;      number of buffer to execute
 */
static int cbuf( int f, int n, int bufnum) {
    /* make the buffer name */
    macbufname[ MACDIGITPOS]     = '0' + (bufnum / 10) ;
    macbufname[ MACDIGITPOS + 1] = '0' + (bufnum % 10) ;

	return exec( n, macbufname, "macro") ;
}

/* execute buffer of numbered macro [1..40] */
#define cbufnn( nn) \
BINDABLE( cbuf##nn) { \
    return cbuf( f, n, nn) ; \
}

cbufnn( 1)
cbufnn( 2)
cbufnn( 3)
cbufnn( 4)
cbufnn( 5)
cbufnn( 6)
cbufnn( 7)
cbufnn( 8)
cbufnn( 9)
cbufnn( 10)
cbufnn( 11)
cbufnn( 12)
cbufnn( 13)
cbufnn( 14)
cbufnn( 15)
cbufnn( 16)
cbufnn( 17)
cbufnn( 18)
cbufnn( 19)
cbufnn( 20)
cbufnn( 21)
cbufnn( 22)
cbufnn( 23)
cbufnn( 24)
cbufnn( 25)
cbufnn( 26)
cbufnn( 27)
cbufnn( 28)
cbufnn( 29)
cbufnn( 30)
cbufnn( 31)
cbufnn( 32)
cbufnn( 33)
cbufnn( 34)
cbufnn( 35)
cbufnn( 36)
cbufnn( 37)
cbufnn( 38)
cbufnn( 39)
cbufnn( 40)

/* end of exec.c */
