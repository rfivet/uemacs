/* exec.c -- implements exec.h */
#include "exec.h"

/*  exec.c
 *
 *  This file is for functions dealing with execution of
 *  commands, command lines, buffers, files and startup files.
 *
 *  written 1986 by Daniel Lawrence
 *  modified by Petri Kutvonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "bind.h"
#include "display.h"
#include "estruct.h"
#include "eval.h"
#include "file.h"
#include "flook.h"
#include "input.h"
#include "line.h"
#include "random.h"
#include "util.h"
#include "window.h"


static char *execstr = NULL ;	/* pointer to string to execute */
boolean clexec = FALSE ;    /* command line execution flag  */



/*  Directive definitions   */

#define DIF     0
#define DELSE       1
#define DENDIF      2
#define DGOTO       3
#define DRETURN     4
#define DENDM       5
#define DWHILE      6
#define DENDWHILE   7
#define DBREAK      8
#define DFORCE      9

#define NUMDIRS     10

/* The !WHILE directive in the execution language needs to
 * stack references to pending whiles. These are stored linked
 * to each currently open procedure via a linked list of
 * the following structure.
*/
struct while_block {
    struct line *w_begin;        /* ptr to !while statement */
    struct line *w_end;          /* ptr to the !endwhile statement */
    int w_type;          /* block type */
    struct while_block *w_next;  /* next while */
};

#define BTWHILE     1
#define BTBREAK     2

/* directive name table:
    This holds the names of all the directives....  */

static const char *dname[] = {
    "if", "else", "endif",
    "goto", "return", "endm",
    "while", "endwhile", "break",
    "force"
};

static char golabel[ NSTRING] = "" ;    /* current line to go to        */
static int execlevel = 0 ;          /* execution IF level           */
static struct buffer *bstore = NULL ;   /* buffer to store macro text to */
static int mstore = FALSE ;         /* storing text to macro flag   */

static int dobuf( struct buffer *bp) ;
static void freewhile( struct while_block *wp) ;
static int macarg( char *tok, int toksz) ;

/*
 * Execute a named command even if it is not bound.
 */
int namedcmd( int f, int n) {
    /* prompt the user to type a named command */
    mlwrite(": ");

    /* and now get the function name to execute */
	const name_bind *nbp = getname() ;
    fnp_t kfunc = nbp->n_func ;
    if (kfunc == NULL) {
        mlwrite("(No such function)");
        return FALSE;
    }

	if( nbp->tag && (curbp->b_mode & MDVIEW))
		return rdonly() ;

    /* and then execute the command */
    return kfunc(f, n) ;
}

static int docmd( char *cline) ;

/*
 * execcmd:
 *  Execute a command line command to be typed in
 *  by the user
 *
 * int f, n;        default Flag and Numeric argument
 */
int execcmd( int f, int n) {
    int status ;	/* status return */
    char *cmdstr ;	/* string holding command to execute */

    /* get the line wanted */
    status = newmlarg( &cmdstr, ": ", 0) ;
    if( status != TRUE)
        return status ;

    execlevel = 0 ;
    while( status == TRUE && n-- > 0)
        status = docmd( cmdstr) ;

    free( cmdstr) ;
    return status ;
}

/*
 * docmd:
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
    int f;      /* default argument flag */
    int n;      /* numeric repeat value */
    int status;     /* return status of function */
    boolean oldcle ;    /* old contents of clexec flag */
    char *oldestr;      /* original exec string */
    char tkn[NSTRING];  /* next token off of command line */

    /* if we are scanning and not executing..go back here */
    if (execlevel)
        return TRUE;

    oldestr = execstr;  /* save last ptr to string to execute */
    execstr = cline;    /* and set this one as current */

    /* first set up the default command values */
    f = FALSE;
    n = 1;
    lastflag = thisflag;
    thisflag = 0;

    status = macarg( tkn, sizeof tkn) ;
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
            execstr = oldestr;
            return status;
        }
    }

    /* and match the token to see if it exists */
	const name_bind *nbp = fncmatch( tkn) ;
    fnp_t fnc = nbp->n_func ;
    if( fnc == NULL) {
        mlwrite("(No such Function)");
        execstr = oldestr;
        return FALSE;
    }

	if( nbp->tag && (curbp->b_mode & MDVIEW))
		status = rdonly() ;
	else {
	    /* save the arguments and go execute the command */
    	oldcle = clexec;    /* save old clexec flag */
    	clexec = TRUE;      /* in cline execution */
	    status = fnc( f, n) ;	/* call the function */
	    clexec = oldcle;    /* restore clexec flag */
    	execstr = oldestr;
	}

    cmdstatus = status; /* save the status */
    return status;
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
    boolean quotef ;	/* is the current string quoted? */
    char	*tok ;		/* allocated string */
    int		size ;		/* allocated size */
    int		idx = 0 ;	/* insertion point into token string */

	tok = malloc( NSTRING) ;
	size = (tok == NULL) ? 0 : NSTRING ;

    /* first scan past any whitespace in the source string */
    while (*src == ' ' || *src == '\t')
        ++src;

    /* scan through the source string */
    quotef = FALSE;
    while (*src) {
	    char c ;	/* temporary character */

        /* process special characters */
        if (*src == '~') {
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

/*
 * storemac:
 *  Set up a macro buffer and flag to store all
 *  executed command lines there
 *
 * int f;       default flag
 * int n;       macro number to use
 */
static char macbufname[] = "*Macro xx*" ;
#define MACDIGITPOS 7

int storemac(int f, int n)
{
    struct buffer *bp;  /* pointer to macro buffer */

    /* must have a numeric argument to this function */
    if (f == FALSE) {
        mlwrite("No macro specified");
        return FALSE;
    }

    /* range check the macro number */
    if (n < 1 || n > 40) {
        mlwrite("Macro number out of range");
        return FALSE;
    }

    /* construct the macro buffer name */
    macbufname[ MACDIGITPOS]     = '0' + (n / 10) ;
    macbufname[ MACDIGITPOS + 1] = '0' + (n % 10) ;

    /* set up the new macro buffer */
    bp = bfind( macbufname, TRUE, BFINVS) ;
    if( bp == NULL) {
        mlwrite("Can not create macro");
        return FALSE;
    }

    /* and make sure it is empty */
    bclear(bp);

    /* and set the macro store pointers to it */
    mstore = TRUE;
    bstore = bp;
    return TRUE;
}

/*
**  exec -- execute a buffer
**	common to execute buffer, procedure and macro
*/
static int exec( int n, char *bufname, char *errstr) {
    struct buffer *bp ;		/* ptr to buffer to execute */

    /* find the pointer to that buffer */
    bp = bfind( bufname, FALSE, 0) ;
    if( bp == NULL) {
        mlwrite( "No such %s", errstr) ;
        return FALSE ;
    }

    /* and now execute it as asked */
    int status = TRUE ;
    while( status == TRUE && n-- > 0)
        status = dobuf( bp) ;

    return status ;
}

#if PROC
/*
 * storeproc:
 *  Set up a procedure buffer and flag to store all
 *  executed command lines there
 *
 * int f;       default flag
 * int n;       macro number to use
 */
int storeproc( int f, int n) {
    struct buffer *bp ;	/* pointer to macro buffer */
    int status ;		/* return status */
    bname_t bname ;		/* name of buffer to use */
    char *name ;

    /* a numeric argument means its a numbered macro */
    if( f == TRUE)
        return storemac( f, n) ;

    /* get the name of the procedure */
    status = newmlarg( &name, "Procedure name: ", sizeof bname - 2) ;
    if( status != TRUE)
        return status ;

    /* construct the macro buffer name */
    bname[ 0] = '*';
    mystrscpy( &bname[ 1], name, sizeof bname - 2) ;
    strcat( bname, "*") ;
	free( name) ;

    /* set up the new macro buffer */
    bp = bfind( bname, TRUE, BFINVS) ;
    if( bp == NULL) {
        mlwrite( "Can not create macro") ;
        return FALSE ;
    }

    /* and make sure it is empty */
    bclear( bp) ;

    /* and set the macro store pointers to it */
    mstore = TRUE ;
    bstore = bp ;
    return TRUE ;
}

/*
 * execproc:
 *  Execute a procedure
 *
 * int f, n;        default flag and numeric arg
 */
int execproc( int f, int n) {
    int status ;			/* status return */
    bname_t bufn ;		/* name of buffer to execute */
    char *name ;

    /* find out what buffer the user wants to execute */
    status = newmlarg( &name, "Execute procedure: ", sizeof bufn - 2) ;
    if( status != TRUE)
        return status ;

    /* construct the buffer name */
    bufn[ 0] = '*' ;
    mystrscpy( &bufn[ 1], name, sizeof bufn - 2) ;
    strcat( bufn, "*") ;
    free( name) ;

	return exec( n, bufn, "procedure") ;
}
#endif

/*
 * execbuf:
 *  Execute the contents of a buffer of commands
 *
 * int f, n;        default flag and numeric arg
 */
int execbuf( int f, int n) {
    int status ;		/* status return */
    char *bufn ;		/* name of buffer to execute */

    /* find out what buffer the user wants to execute */
    status = newmlarg( &bufn, "Execute buffer: ", sizeof( bname_t)) ;
    if( status != TRUE)
        return status ;

    status = exec( n, bufn, "buffer") ;
    free( bufn) ;
    return status ;
}

/*
 * dobuf:
 *  execute the contents of the buffer pointed to
 *  by the passed BP
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
 *  Line Labels begin with a "*" as the first nonblank char, like:
 *
 *  *LBL01
 *
 * struct buffer *bp;       buffer to execute
 */
static int dobuf(struct buffer *bp)
{
    int status; /* status return */
    struct line *lp;    /* pointer to line to execute */
    struct line *hlp;   /* pointer to line header */
    struct line *glp;   /* line to goto */
    struct line *mp;        /* Macro line storage temp */
    int dirnum;     /* directive index */
    int linlen;     /* length of line to execute */
    int i;          /* index */
    int force;      /* force TRUE result? */
    struct window *wp;      /* ptr to windows to scan */
    struct while_block *whlist; /* ptr to !WHILE list */
    struct while_block *scanner;    /* ptr during scan */
    struct while_block *whtemp; /* temporary ptr to a struct while_block */
    char *einit;        /* initial value of eline */
    char *eline;        /* text of line to execute */
    char tkn[NSTRING];  /* buffer to evaluate an expresion in */

    /* clear IF level flags/while ptr */
    execlevel = 0;
    whlist = NULL;
    scanner = NULL;

    /* scan the buffer to execute, building WHILE header blocks */
    hlp = bp->b_linep;
    lp = hlp->l_fp;
    while (lp != hlp) {
        /* scan the current line */
        eline = lp->l_text;
        i = lp->l_used;

        /* trim leading whitespace */
        while (i-- > 0 && (*eline == ' ' || *eline == '\t'))
            ++eline;

        /* if theres nothing here, don't bother */
        if (i <= 0)
            goto nxtscan;

        /* if is a while directive, make a block... */
        if (eline[0] == '!' && eline[1] == 'w' && eline[2] == 'h') {
            whtemp = (struct while_block *)malloc(sizeof(struct while_block));
            if (whtemp == NULL) {
                  noram:mlwrite
                    ("%%Out of memory during while scan");
                  failexit:freewhile
                    (scanner);
                freewhile(whlist);
                return FALSE;
            }
            whtemp->w_begin = lp;
            whtemp->w_type = BTWHILE;
            whtemp->w_next = scanner;
            scanner = whtemp;
        }

        /* if is a BREAK directive, make a block... */
        if (eline[0] == '!' && eline[1] == 'b' && eline[2] == 'r') {
            if (scanner == NULL) {
                mlwrite
                    ("%%!BREAK outside of any !WHILE loop");
                goto failexit;
            }
            whtemp = (struct while_block *)malloc(sizeof(struct while_block));
            if (whtemp == NULL)
                goto noram;
            whtemp->w_begin = lp;
            whtemp->w_type = BTBREAK;
            whtemp->w_next = scanner;
            scanner = whtemp;
        }

        /* if it is an endwhile directive, record the spot... */
        if (eline[0] == '!' && strncmp(&eline[1], "endw", 4) == 0) {
            if (scanner == NULL) {
                mlwrite
                    ("%%!ENDWHILE with no preceding !WHILE in '%s'",
                     bp->b_bname);
                goto failexit;
            }
            /* move top records from the scanner list to the
               whlist until we have moved all BREAK records
               and one WHILE record */
            do {
                scanner->w_end = lp;
                whtemp = whlist;
                whlist = scanner;
                scanner = scanner->w_next;
                whlist->w_next = whtemp;
            } while (whlist->w_type == BTBREAK);
        }

          nxtscan:      /* on to the next line */
        lp = lp->l_fp;
    }

    /* while and endwhile should match! */
    if (scanner != NULL) {
        mlwrite("%%!WHILE with no matching !ENDWHILE in '%s'",
            bp->b_bname);
        goto failexit;
    }

    /* let the first command inherit the flags from the last one.. */
    thisflag = lastflag;

    /* starting at the beginning of the buffer */
    hlp = bp->b_linep;
    lp = hlp->l_fp;
    while (lp != hlp) {
        /* allocate eline and copy macro line to it */
        linlen = lp->l_used;
        if ((einit = eline = malloc(linlen + 1)) == NULL) {
            mlwrite("%%Out of Memory during macro execution");
            freewhile(whlist);
            return FALSE;
        }

        mystrscpy( eline, lp->l_text, linlen + 1) ;

        /* trim leading whitespace */
        while (*eline == ' ' || *eline == '\t')
            ++eline;

        /* dump comments and blank lines */
        if (*eline == ';' || *eline == '#' || *eline == 0)
            goto onward;

#if DEBUGM
        /* if $debug == TRUE, every line to execute
           gets echoed and a key needs to be pressed to continue
           ^G will abort the command */

		if( macbug) {
			int c ;
        	
            /* debug macro name, if levels and lastly the line */
			c = mdbugout( "<<<%s:%d:%s>>>", bp->b_bname, execlevel, eline) ;
        	if( c == abortc) {
                freewhile( whlist) ;
				free( einit) ;
                return FALSE ;
        	} else if( c == metac) {
        		macbug = FALSE ;
        	}
        }
#endif

        /* Parse directives here.... */
        dirnum = -1;
        if (*eline == '!') {
            /* Find out which directive this is */
            ++eline;
            for (dirnum = 0; dirnum < NUMDIRS; dirnum++)
                if (strncmp(eline, dname[dirnum],
                        strlen(dname[dirnum])) == 0)
                    break;

            /* and bitch if it's illegal */
            if (dirnum == NUMDIRS) {
                mlwrite("%%Unknown Directive");
                freewhile(whlist);
				free( einit) ;
                return FALSE;
            }

            /* service only the !ENDM macro here */
            if (dirnum == DENDM) {
                mstore = FALSE;
                bstore = NULL;
                goto onward;
            }

            /* restore the original eline.... */
            --eline;
        }

        /* if macro store is on, just salt this away */
        if (mstore) {
            /* allocate the space for the line */
            linlen = strlen(eline);
            if ((mp = lalloc(linlen)) == NULL) {
				free( einit) ;
                mlwrite( "Out of memory while storing macro") ;
                return FALSE ;
            }

            /* copy the text into the new line */
            for (i = 0; i < linlen; ++i)
                lputc(mp, i, eline[i]);

            /* attach the line to the end of the buffer */
            bstore->b_linep->l_bp->l_fp = mp;
            mp->l_bp = bstore->b_linep->l_bp;
            bstore->b_linep->l_bp = mp;
            mp->l_fp = bstore->b_linep;
            goto onward;
        }


        force = FALSE;

        /* dump comments */
        if (*eline == '*')
            goto onward;

        /* now, execute directives */
        if (dirnum != -1) {
            /* skip past the directive */
            while (*eline && *eline != ' ' && *eline != '\t')
                ++eline;
            execstr = eline;

            switch (dirnum) {
            case DIF:   /* IF directive */
                /* grab the value of the logical exp */
                if (execlevel == 0) {
                    if( macarg( tkn, sizeof tkn) != TRUE)
                        goto eexec;
                    if (stol(tkn) == FALSE)
                        ++execlevel;
                } else
                    ++execlevel;
                goto onward;

            case DWHILE:    /* WHILE directive */
                /* grab the value of the logical exp */
                if (execlevel == 0) {
                    if( macarg( tkn, sizeof tkn) != TRUE)
                        goto eexec;
                    if (stol(tkn) == TRUE)
                        goto onward;
                }
                /* drop down and act just like !BREAK */

				/* fallthrough */
            case DBREAK:    /* BREAK directive */
                if (dirnum == DBREAK && execlevel)
                    goto onward;

                /* jump down to the endwhile */
                /* find the right while loop */
                whtemp = whlist;
                while (whtemp) {
                    if (whtemp->w_begin == lp)
                        break;
                    whtemp = whtemp->w_next;
                }

                if (whtemp == NULL) {
                    mlwrite
                        ("%%Internal While loop error");
                    freewhile(whlist);
                    return FALSE;
                }

                /* reset the line pointer back.. */
                lp = whtemp->w_end;
                goto onward;

            case DELSE: /* ELSE directive */
                if (execlevel == 1)
                    --execlevel;
                else if (execlevel == 0)
                    ++execlevel;
                goto onward;

            case DENDIF:    /* ENDIF directive */
                if (execlevel)
                    --execlevel;
                goto onward;

            case DGOTO: /* GOTO directive */
                /* .....only if we are currently executing */
                if (execlevel == 0) {
                    /* grab label to jump to */
                    eline = token( eline, golabel, sizeof golabel) ;
                    linlen = strlen(golabel);
                    glp = hlp->l_fp;
                    while (glp != hlp) {
                        if (*glp->l_text == '*' &&
                            (strncmp
                             (&glp->l_text[1],
                              golabel,
                              linlen) == 0)) {
                            lp = glp;
                            goto onward;
                        }
                        glp = glp->l_fp;
                    }
                    mlwrite("%%No such label");
                    freewhile(whlist);
                    return FALSE;
                }
                goto onward;

            case DRETURN:   /* RETURN directive */
                if (execlevel == 0)
                    goto eexec;
                goto onward;

            case DENDWHILE: /* ENDWHILE directive */
                if (execlevel) {
                    --execlevel;
                    goto onward;
                } else {
                    /* find the right while loop */
                    whtemp = whlist;
                    while (whtemp) {
                        if (whtemp->w_type ==
                            BTWHILE
                            && whtemp->w_end == lp)
                            break;
                        whtemp = whtemp->w_next;
                    }

                    if (whtemp == NULL) {
                        mlwrite
                            ("%%Internal While loop error");
                        freewhile(whlist);
                        return FALSE;
                    }

                    /* reset the line pointer back.. */
                    lp = whtemp->w_begin->l_bp;
                    goto onward;
                }

            case DFORCE:    /* FORCE directive */
                force = TRUE;

            }
        }

        /* execute the statement */
        status = docmd(eline);
        if (force)  /* force the status */
            status = TRUE;

        /* check for a command error */
        if (status != TRUE) {
            /* look if buffer is showing */
            wp = wheadp;
            while (wp != NULL) {
                if (wp->w_bufp == bp) {
                    /* and point it */
                    wp->w_dotp = lp;
                    wp->w_doto = 0;
                    wp->w_flag |= WFHARD;
                }
                wp = wp->w_wndp;
            }
            /* in any case set the buffer . */
            bp->b_dotp = lp;
            bp->b_doto = 0;
            free(einit);
            execlevel = 0;
            freewhile(whlist);
            return status;
        }

          onward:       /* on to the next line */
        free(einit);
        lp = lp->l_fp;
    }

      eexec:            /* exit the current function */
    execlevel = 0;
    freewhile(whlist);
    return TRUE;
}

/*
 * free a list of while block pointers
 *
 * struct while_block *wp;      head of structure to free
 */
static void freewhile(struct while_block *wp)
{
    if (wp == NULL)
        return;
    if (wp->w_next)
        freewhile(wp->w_next);
    free(wp);
}

/*
 * execute a series of commands in a file
 *
 * int f, n;        default flag and numeric arg to pass on to file
 */
int execfile( int f, int n) {
    int status ;	/* return status of name query */
    char *fname ;	/* name of file to execute */
    char *fspec ;	/* full file spec */

    status = newmlarg( &fname, "Execute file: ", 0) ;
    if( status != TRUE)
        return status ;

/* look up the path for the file */
    fspec = flook( fname, FALSE) ;    /* used to be TRUE, P.K. */
    free( fname) ;

/* if it isn't around */
    if( fspec == NULL)
        return FALSE ;

/* otherwise, execute it */
    while( status == TRUE && n-- > 0)
        status = dofile( fspec) ;

    return status ;
}

/*
 * dofile:
 *  yank a file into a buffer and execute it
 *  if there are no errors, delete the buffer on exit
 *
 * char *fname;     file name to execute
 */
int dofile( const char *fname) {
    struct buffer *bp;  /* buffer to place file to exeute */
    struct buffer *cb;  /* temp to hold current buf while we read */
    int status; /* results of various calls */
    bname_t bname ; /* name of buffer */

    makename(bname, fname); /* derive the name of the buffer */
    unqname(bname);     /* make sure we don't stomp things */
    if ((bp = bfind(bname, TRUE, 0)) == NULL)   /* get the needed buffer */
        return FALSE;

    bp->b_mode = MDVIEW;    /* mark the buffer as read only */
    cb = curbp;     /* save the old buffer */
    curbp = bp;     /* make this one current */
    /* and try to read in the file to execute */
    if ((status = readin(fname, FALSE)) != TRUE) {
        curbp = cb; /* restore the current buffer */
        return status;
    }

    /* go execute it! */
    curbp = cb;     /* restore the current buffer */
    if ((status = dobuf(bp)) != TRUE)
        return status;

    /* if not displayed, remove the now unneeded buffer and exit */
    if (bp->b_nwnd == 0)
        zotbuf(bp);
    return TRUE;
}

/*
 * cbuf:
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
int cbuf##nn( int f, int n) { \
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
