/* bind.c -- implements bind.h */
#include "bind.h"

/*  This file is for functions having to do with key bindings,
 *  descriptions, help commands and startup file.
 *
 *  Written 11-feb-86 by Daniel Lawrence
 *  Modified by Petri Kutvonen
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estruct.h"
#include "bindable.h"
#include "buffer.h"
#include "display.h"
#include "exec.h"
#include "file.h"
#include "flook.h"
#include "input.h"
#include "line.h"
#include "names.h"
#include "util.h"
#include "window.h"


static int buildlist( char *mstring) ;
static char *cmdstr( unsigned c, char *seq) ;
static unsigned int getckey( int mflag) ;
static unsigned int stock( char *keyname) ;
static const char *getfname( unsigned keycode, const char *failmsg) ;


static boolean cmdfail( const char *msg) {
	mlwrite( "%s", msg) ;
	return FALSE ;
}

BINDABLE( help) {
/* give me some help!!!!
   bring up a fake buffer and read the help file into it with view mode */
    char *fname = NULL; /* ptr to file returned by flook() */

/* first check if we are already here */
    buffer_p bp = bfind( hlpfname, FALSE, BFINVS);
	if( bp == curbp)
		return TRUE ;

    if( bp == NULL) {
        fname = flook( hlpfname, FALSE) ;
        if( fname == NULL)
			return cmdfail( "(Help file is not online)") ;
    }

/* split the current window to make room for the help stuff */
	if( wheadp->w_wndp == NULL			/* One window */
	&&  splitwind( FALSE, 1) == FALSE)	/* Split it */
        return FALSE ;

    if (bp == NULL) {
        /* and read the stuff in */
        if (getfile(fname, FALSE) == FALSE)
            return FALSE;
    } else
		swbuffer( bp) ;

    /* make this window in VIEW mode, update all mode lines */
    curwp->w_bufp->b_mode |= MDVIEW;
    curwp->w_bufp->b_flag |= BFINVS;
    upmode() ;
    return TRUE;
}

static boolean invalidkey( void) {
	return cmdfail( "(Invalid key sequence)") ;
}

/* describe the command for a certain key */
BINDABLE( deskey) {
	const char cmdname[] = "describe-key" ;
    char outseq[ NSTRING] ;	/* output buffer for command sequence */

/* prompt the user to type a key to describe */
    mlwrite( "%s: ", cmdname) ;

/* get the command sequence to describe
 * change it to something we can print as well */
    unsigned keycode = getckey( FALSE) ;
	if( keycode == (unsigned) ~0)
		return invalidkey() ;

/* output the command sequence */
    mlwrite( "%s %s: 0x%x, %s", cmdname, cmdstr( keycode, outseq), keycode,
                                            getfname( keycode, "Not Bound")) ;
    return TRUE ;
}

/*
 * bindtokey:
 *  add a new key to the key binding table
 *
 * int f, n;        command arguments [IGNORED]
 */
BINDABLE( bindtokey) {
    kbind_p ktp ;		/* pointer into the command table */
    char outseq[ 80] ;  /* output buffer for keystroke sequence */

/* prompt the user to type in a key to bind */
    mlwrite("bind-to-key: ");

/* get the function name to bind it to */
	nbind_p nbp = getname() ;
	if( nbp == NULL)	/* abort */
		return FALSE ;

    fnp_t kfunc = nbp->n_func ;
    if( kfunc == NULL)
		return cmdfail( "(No such function)") ;

	mlwrite( "bind-to-key %s: ", bind_name( nbp)) ;

/* get the command sequence to bind */
	boolean prefix_f = (kfunc == metafn) || (kfunc == cex) ||
            							(kfunc == unarg) || (kfunc == ctrlg) ;
    int c = getckey( prefix_f) ;
	if( c == ~0)
		return invalidkey() ;

/* change it to something we can print as well */
/* and dump it out */
    ostring( cmdstr( c, outseq)) ;

/* key sequence can't be an active prefix key */
	if( c == metac || c == ctlxc || c == reptc || c == abortc) {
		if( (c == metac  && kfunc == metafn)
		||  (c == ctlxc  && kfunc == cex)
		||  (c == reptc  && kfunc == unarg)
		||  (c == abortc && kfunc == ctrlg))
			return TRUE ;

		return cmdfail( "(Can't bind to active prefix)") ;
	}

/* if the function is a prefix key */
	if( prefix_f) {
    /* remove existing binding for the prefix */
        for( ktp = keytab ; ktp->k_code != 0 ; ktp++)
            if( ktp->k_nbp == nbp) {
				delkeybinding( ktp->k_code) ;
				break ;
			}

    /* set the appropriate global prefix variable */
        if( kfunc == metafn)
            metac = c ;
        else if( kfunc == cex)
            ctlxc = c ;
        if( kfunc == unarg)
            reptc = c ;
        if( kfunc == ctrlg)
            abortc = c ;
    }

	ktp = setkeybinding( c, nbp) ;
	if( ktp->k_code == 0)
		return cmdfail( "Binding table FULL!") ;

	return TRUE ;
}

/*
 * unbindkey:
 *  delete a key from the key binding table
 *
 * int f, n;        command arguments [IGNORED]
 */
BINDABLE( unbindkey) {
    char outseq[ 80] ;	/* output buffer for keystroke sequence */

/* prompt the user to type in a key to unbind */
    mlwrite( "unbind-key: ") ;

/* get the command sequence to unbind */
    int c = getckey( FALSE) ;	/* get a command sequence */
	if( c == ~0)
		return invalidkey() ;

/* change it to something we can print as well */
/* and dump it out */
    ostring( cmdstr( c, outseq)) ;

/* prefix key sequence can't be undound, just redefined */
	if( c == reptc || c == abortc)
		return cmdfail( "(Can't unbind prefix)") ;

/* if it isn't bound, bitch */
    if( delkeybinding( c) == FALSE) {
        mlwrite( "(Key not bound)") ;
        return FALSE ;
    }
	
	return TRUE ;
}

/*
 * does source include sub?
 *
 * char *source;    string to search in
 * char *sub;       substring to look for
 */
static boolean strinc( const char *source, const char *sub) {
    /* for each character in the source string */
    for( ; *source ; source++) {
		const char *nxtsp ;  /* next ptr into source */
		const char *tp ;	/* ptr into substring */

        nxtsp = source;

        /* is the substring here? */
        for( tp = sub ; *tp ; tp++)
            if( *nxtsp++ != *tp)
                break ;

        /* yes, return a success */
        if( *tp == 0)
            return TRUE ;
    }

    return FALSE ;
}

/* describe bindings
 * bring up a fake buffer and list the key bindings
 * into it with view mode
 */
BINDABLE( desbind) {
    return buildlist( "") ;
}

/* Apropos (List functions that match a substring) */
BINDABLE( apro) {
	char *mstring ;	/* string to match cmd names to */

    int status = newmlarg( &mstring, "apropos: ", 0) ;
	if( status == TRUE) {
		status = buildlist( mstring) ;
		free( mstring) ;
	} else if( status == FALSE)
		status = buildlist( "") ;	/* build list of all commands */

    return status ;
}

/*
 * build a binding list (limited or full)
 *
 * char *mstring;   match string if a partial list, "" matches all
 */
static int buildlist( char *mstring) {
    struct window *wp;         /* scanning pointer to windows */
    kbind_p ktp;  /* pointer into the command table */
    nbind_p nptr;/* pointer into the name binding table */
    struct buffer *bp;    /* buffer to put binding list into */
    char outseq[80];      /* output buffer for keystroke sequence */

    /* split the current window to make room for the binding list */
	if( wheadp->w_wndp == NULL			/* One window */
	&&  splitwind( FALSE, 1) == FALSE)	/* Split it */
        return FALSE;

    /* and get a buffer for it */
    bp = bfind("*Binding list*", TRUE, 0);
    if( bp == NULL || bclear( bp) == FALSE)
		return cmdfail( "Can't display binding list") ;

    /* let us know this is in progress */
    mlwrite("(Building binding list)");

    /* disconnect the current buffer */
    if (--curbp->b_nwnd == 0) { /* Last use.            */
        curbp->b_dotp = curwp->w_dotp;
        curbp->b_doto = curwp->w_doto;
        curbp->b_markp = curwp->w_markp;
        curbp->b_marko = curwp->w_marko;
    }

    /* connect the current window to this buffer */
    curbp = bp;     /* make this buffer current in current window */
    bp->b_mode = 0;     /* no modes active in binding list */
    bp->b_nwnd++;       /* mark us as more in use */
    wp = curwp;
    wp->w_bufp = bp;
    wp->w_linep = bp->b_linep;
    wp->w_flag = WFHARD | WFFORCE;
    wp->w_dotp = bp->b_dotp;
    wp->w_doto = bp->b_doto;
    wp->w_markp = NULL;
    wp->w_marko = 0;

    /* build the contents of this window, inserting it line by line */
    for( nptr = names ; nptr->n_func != NULL ; nptr++) {
	    int cpos ;	/* current position to use in outseq */

        /* if we are executing an apropos command..... */
        /* and current string doesn't include the search string */
        if( *mstring && strinc( bind_name( nptr), mstring) == FALSE)
			continue ;

        /* add in the command name */
        mystrscpy( outseq, bind_name( nptr), sizeof outseq) ;
        cpos = strlen(outseq);

        /* search down any keys bound to this */
        for( ktp = keytab ; ktp->k_code != 0 ; ktp++) {
            if( ktp->k_nbp == nptr) {
                /* padd out some spaces */
                while (cpos < 28)
                    outseq[cpos++] = ' ';

                /* add in the command sequence */
                cmdstr(ktp->k_code, &outseq[cpos]);
                strcat(outseq, "\n");

                /* and add it as a line into the buffer */
                if (linstr(outseq) != TRUE)
                    return FALSE;

                cpos = 0;   /* and clear the line */
            }
        }

        /* if no key was bound, we need to dump it anyway */
        if (cpos > 0) {
            outseq[cpos++] = '\n';
            outseq[cpos] = 0;
            if (linstr(outseq) != TRUE)
                return FALSE;
        }
    }

    bp->b_mode |= MDVIEW;    /* put this buffer view mode */
    bp->b_flag &= ~BFCHG;    /* don't flag this as a change */
    wp->w_dotp = lforw(bp->b_linep);    /* back to the beginning */
    wp->w_doto = 0;
    upmode() ;			/* and update ALL mode lines */
    mlwrite("");        /* clear the mode line */
    return TRUE;
}

/*
 * get a command key sequence from the keyboard
 *
 * int mflag;       going for a meta sequence?
 * returns ~0 on failure
 */
static unsigned int getckey( int mflag) {
    unsigned int c ;	/* character fetched */

    /* check to see if we are executing a command line */
    if( clexec) {
		char *tok = getnewtokval() ;	/* get the next token */
		if( tok == NULL)
			c = ~0 ;	/* return invalid key on failure */
		else {
			c = stock( tok) ;
			free( tok) ;
		}
    } else {	/* or the normal way */
	    if( mflag)
    	    c = get1key() ;
    	else
			c = getcmd() ;
	}

	return c ;
}

/*
 * execute the startup file
 *
 * char *fname;    name of startup file (null if default)
 */
int startup( const char *fname) {
	if( !fname || *fname == 0)		/* use default if empty parameter */
		fname = rcfname ;

	fname = flook( fname, TRUE) ;	/* look up the startup file */
	if( fname == NULL)				/* if it isn't around, don't sweat it */
		return TRUE ;

	return dofile( fname) ;			/* otherwise, execute the sucker */
}

/*
 * change a key command to a string we can print out
 *
 * int c;       sequence to translate
 * char *seq;       destination string for sequence
 */
static char *cmdstr( unsigned c, char *seq) {
    char *ptr = seq ;	/* pointer into current position in sequence */

/* apply meta sequence if needed */
    if( c & META) {
        *ptr++ = 'M';
        *ptr++ = '-';
    }

/* apply ^X sequence if needed */
    if( c & CTLX) {
		if( ctlxc & CTRL)
	        *ptr++ = '^' ;

        *ptr++ = ctlxc & ~PRFXMASK ;
    }

/* apply control sequence if needed */
    if( c & CTRL)
        *ptr++ = '^' ;

/* apply SPEC sequence if needed */
    if( c & SPEC) {
        *ptr++ = 'F' ;
        *ptr++ = 'N' ;
    }

/* and output the final sequence */
	ptr += unicode_to_utf8( c & ~PRFXMASK, ptr) ;
    *ptr = 0 ;			/* terminate the string */
	return seq ;
}

static const char *getfname( unsigned keycode, const char *failmsg) {
/* takes a key code and gets the name of the function bound to it */
	kbind_p kbp = getkeybinding( keycode) ;
	if( kbp->k_code == 0)
		return failmsg ;

	const char *found = bind_name( kbp->k_nbp) ;
	assert( *found) ;
	return found ;
}

/* stock:
 *  String key name TO Command Key
 *
 * char *keyname;   name of key to translate to Command key form
 * fmt: [M-|^X][^][FN]X
 * returns ~0 on invalid sequence
 */
static unsigned int stock( char *keyname) {
/* parse it up */
    unsigned c = 0 ;

/* first, the prefix META or ^X */
    if( *keyname == 'M' && keyname[ 1] == '-') {
        c = META ;
        keyname += 2 ;
    } else if( *keyname == '^' && keyname[ 1] == 'X') {
        c = CTLX ;
        keyname += 2 ;
    }

/* a control char? */
    if( *keyname == '^' && keyname[ 1] != 0) {
        c |= CTRL ;
        ++keyname ;
    }

/* next the function prefix */
    if( *keyname == 'F' && keyname[ 1] == 'N') {
        c |= SPEC ;
        keyname += 2 ;
    }

/* only one character left to parse */
	if( !*keyname || keyname[1])
		return ~0 ;

/* only way to redefine ^X is by quoting binary value */
    if( *keyname < 32 || *keyname == 0x7F) {
        c |= CTRL ;
        *keyname ^= 0x40 ;
    } else	if( c && !(c & SPEC)
			&&	*keyname >= 'a' && *keyname <= 'z')
	/* make sure we are not lower case (not with function keys) */
        *keyname -= 32 ;

/* the final sequence... */
    c |= *keyname & 0xFFU ;
    return c ;
}

/*
 * string key name to binding name....
 *
 * char *skey;      name of key to get binding for
 */
const char *transbind( char *skey) {
	static const char failmsg[] = "ERROR" ;

	unsigned c = stock( skey) ;
	if( c == (unsigned) ~0)
		return failmsg ;
	else
	    return getfname( c, failmsg) ;
}

/* end of bind.c */
