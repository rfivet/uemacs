/* bind.c -- implements bind.h */
#include "bind.h"

/*  bind.c
 *
 *  This file is for functions having to do with key bindings,
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


#if APROP
static int buildlist( char *mstring) ;
#endif

static void cmdstr( int c, char *seq) ;
static unsigned int getckey( int mflag) ;
static unsigned int stock( char *keyname) ;
static boolean unbindchar( unsigned c) ;
static const char *getfname( unsigned keycode, char *failmsg) ;


int help(int f, int n)
{               /* give me some help!!!!
                   bring up a fake buffer and read the help file
                   into it with view mode                 */
    struct buffer *bp;  /* buffer pointer to help */
    char *fname = NULL; /* ptr to file returned by flook() */

    /* first check if we are already here */
    bp = bfind( hlpfname, FALSE, BFINVS);

    if (bp == NULL) {
        fname = flook( hlpfname, FALSE);
        if (fname == NULL) {
            mlwrite("(Help file is not online)");
            return FALSE;
        }
    }

    /* split the current window to make room for the help stuff */
    if (splitwind(FALSE, 1) == FALSE)
        return FALSE;

    if (bp == NULL) {
        /* and read the stuff in */
        if (getfile(fname, FALSE) == FALSE)
            return FALSE;
    } else
        swbuffer(bp);

    /* make this window in VIEW mode, update all mode lines */
    curwp->w_bufp->b_mode |= MDVIEW;
    curwp->w_bufp->b_flag |= BFINVS;
    upmode() ;
    return TRUE;
}

/* describe the command for a certain key */
BINDABLE( deskey) {
    char outseq[ NSTRING] ;	/* output buffer for command sequence */

/* prompt the user to type a key to describe */
    mlwrite( "describe-key: ");

/* get the command sequence to describe
 * change it to something we can print as well */
    int c = getckey( FALSE) ;
    cmdstr( c, outseq) ;

/* output the command sequence */
	mlwrite( "describe-key %s: 0x%x, %s",
			  outseq, c, getfname( c, "Not Bound")) ;
    return TRUE ;
}

/*
 * bindtokey:
 *  add a new key to the key binding table
 *
 * int f, n;        command arguments [IGNORED]
 */
BINDABLE( bindtokey) {
    key_tab *ktp ;		/* pointer into the command table */
    char outseq[ 80] ;  /* output buffer for keystroke sequence */

/* prompt the user to type in a key to bind */
    mlwrite("bind-to-key: ");

/* get the function name to bind it to */
	const name_bind *nbp = getname() ;
	if( nbp == NULL)	/* abort */
		return FALSE ;

    fnp_t kfunc = nbp->n_func ;
    if( kfunc == NULL) {
        mlwrite( "(No such function)") ;
        return FALSE ;
    }

	mlwrite( "bind-to-key %s: ", bind_name( nbp)) ;

/* get the command sequence to bind */
	boolean prefix_f = (kfunc == metafn) || (kfunc == cex) ||
            							(kfunc == unarg) || (kfunc == ctrlg) ;
    unsigned c = getckey( prefix_f) ;

/* change it to something we can print as well */
    cmdstr( c, outseq) ;

/* and dump it out */
    ostring( outseq) ;

/* if the function is a prefix key */
	if( prefix_f) {
    /* search and remove all existing binding for the prefix */
        for( ktp = keytab ; ktp->k_code != 0 ; ktp++)
            if( ktp->k_nbp == nbp)
                unbindchar( ktp->k_code) ;

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

/* search the table to see if it exists */
    for( ktp = keytab ; ktp->k_code != 0 ; ktp++) {
        if( ktp->k_code == c) {
		/* it exists, just change it then */
			ktp->k_nbp = nbp ;
			return TRUE ;
        }
    }

/* otherwise we need to add it to the end */
    /* if we run out of binding room, bitch */
    if( ktp >= &keytab[ NBINDS]) {
        mlwrite( "Binding table FULL!") ;
        return FALSE ;
    }

    ktp->k_code = c;    /* add keycode */
	ktp->k_nbp = nbp ;
    ++ktp;      /* and make sure the next is null */
    ktp->k_code = 0;
	ktp->k_nbp = NULL ;

    return TRUE;
}

/*
 * unbindkey:
 *  delete a key from the key binding table
 *
 * int f, n;        command arguments [IGNORED]
 */
int unbindkey(int f, int n)
{
    int c;      /* command key to unbind */
    char outseq[80];    /* output buffer for keystroke sequence */

    /* prompt the user to type in a key to unbind */
    mlwrite("unbind-key: ");

    /* get the command sequence to unbind */
    c = getckey(FALSE); /* get a command sequence */

    /* change it to something we can print as well */
    cmdstr(c, &outseq[0]);

    /* and dump it out */
    ostring(outseq);

    /* if it isn't bound, bitch */
    if (unbindchar(c) == FALSE) {
        mlwrite("(Key not bound)");
        return FALSE;
    }
    return TRUE;
}


/*
 * unbindchar()
 *
 * int c;       command key to unbind
 */
static boolean unbindchar( unsigned c) {
    key_tab *ktp ;   /* pointer into the command table */

/* search the table to see if the key exists */
    for( ktp = keytab ; ktp->k_code != 0 ; ktp++) {
        if (ktp->k_code == c) {
	    /* save the pointer and scan to the end of the table */
    		key_tab *sav_ktp = ktp ;
    		while( (++ktp)->k_code != 0) ;
    		ktp -= 1 ;          /* backup to the last legit entry */

    	/* copy the last entry to the current one */
    		sav_ktp->k_code = ktp->k_code ;
		    sav_ktp->k_nbp  = ktp->k_nbp ;

	    /* null out the last one */
		    ktp->k_code = 0 ;
		    ktp->k_nbp  = NULL ;
		    return TRUE ;
        }
    }

	return FALSE ;
}

#if APROP
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
#endif

/* describe bindings
 * bring up a fake buffer and list the key bindings
 * into it with view mode
 */
int desbind( int f, int n) {
#if APROP
    return buildlist( "") ;
}

/* Apropos (List functions that match a substring) */
int apro( int f, int n) {
	char *mstring ;	/* string to match cmd names to */
	int status ;	/* status return */

    status = newmlarg( &mstring, "apropos: ", 0) ;
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
#endif
    struct window *wp;         /* scanning pointer to windows */
    key_tab *ktp;  /* pointer into the command table */
    const name_bind *nptr;/* pointer into the name binding table */
    struct buffer *bp;    /* buffer to put binding list into */
    char outseq[80];      /* output buffer for keystroke sequence */

    /* split the current window to make room for the binding list */
    if (splitwind(FALSE, 1) == FALSE)
        return FALSE;

    /* and get a buffer for it */
    bp = bfind("*Binding list*", TRUE, 0);
    if (bp == NULL || bclear(bp) == FALSE) {
        mlwrite("Can not display binding list");
        return FALSE;
    }

    /* let us know this is in progress */
    mlwrite("(Building binding list)");

    /* disconect the current buffer */
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

#if APROP
        /* if we are executing an apropos command..... */
            /* and current string doesn't include the search string */
        if( *mstring && strinc( bind_name( nptr), mstring) == FALSE)
			continue ;
#endif
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
 */
static unsigned int getckey( int mflag) {
    unsigned int c ;	/* character fetched */

    /* check to see if we are executing a command line */
    if( clexec) {
		char *tok ;	/* command incoming */
		
		tok = getnewtokval() ;	/* get the next token */
		if( tok == NULL)
			c = 0 ;	/* return dummy key on failure */
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
static void cmdstr( int c, char *seq) {
    char *ptr;      /* pointer into current position in sequence */

    ptr = seq;

    /* apply meta sequence if needed */
    if (c & META) {
        *ptr++ = 'M';
        *ptr++ = '-';
    }

    /* apply ^X sequence if needed */
    if (c & CTLX) {
        *ptr++ = '^';
        *ptr++ = 'X';
    }

    /* apply SPEC sequence if needed */
    if (c & SPEC) {
        *ptr++ = 'F';
        *ptr++ = 'N';
    }

    /* apply control sequence if needed */
    if (c & CONTROL) {
        *ptr++ = '^';
    }

    /* and output the final sequence */

    *ptr++ = c & 255;   /* strip the prefixes */

    *ptr = 0;       /* terminate the string */
}

/*
 * This function looks a key binding up in the binding table
 *
 * int c;       key to find what is bound to it
 */
key_tab *getkeybind( unsigned c) {
    key_tab *ktp ;

    for( ktp = keytab ; ktp->k_code != 0 ; ktp++)
        if (ktp->k_code == c)
			break ;

    return ktp ;
}

static const char *getfname( unsigned keycode, char *failmsg) {
/* takes a key code and gets the name of the function bound to it */
	key_tab *kbp = getkeybind( keycode) ;
	if( kbp->k_code == 0)
		return failmsg ;

	const char *found = bind_name( kbp->k_nbp) ;
	assert( *found) ;
	return found ;
}

/*
 * stock:
 *  String key name TO Command Key
 *
 * char *keyname;   name of key to translate to Command key form
 */
static unsigned int stock( char *keyname) {
    unsigned int c; /* key sequence to return */

    /* parse it up */
    c = 0;

    /* first, the META prefix */
    if (*keyname == 'M' && *(keyname + 1) == '-') {
        c = META;
        keyname += 2;
    }

    /* next the function prefix */
    if (*keyname == 'F' && *(keyname + 1) == 'N') {
        c |= SPEC;
        keyname += 2;
    }

    /* control-x as well... (but not with FN) */
    if (*keyname == '^' && *(keyname + 1) == 'X' && !(c & SPEC)) {
        c |= CTLX;
        keyname += 2;
    }

    /* a control char? */
    if (*keyname == '^' && *(keyname + 1) != 0) {
        c |= CONTROL;
        ++keyname;
    }
    if (*keyname < 32) {
        c |= CONTROL;
        *keyname += 'A';
    }


    /* make sure we are not lower case (not with function keys) */
    if (*keyname >= 'a' && *keyname <= 'z' && !(c & SPEC))
        *keyname -= 32;

    /* the final sequence... */
    c |= *keyname & 0xFFU ;
    return c;
}

/*
 * string key name to binding name....
 *
 * char *skey;      name of key to get binding for
 */
const char *transbind( char *skey) {
    return getfname( stock( skey), "ERROR") ;
}
