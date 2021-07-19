/* execute.c -- implements execute.h */
#include "execute.h"

#define	CLRMSG	0  /* space clears the message line with no insert */

#include <stdlib.h>
#include <unistd.h>

#include "estruct.h"
#include "bind.h"
#include "random.h"
#include "display.h"
#include "file.h"
#include "input.h"
#include "mlout.h"
#include "search.h"
#include "terminal.h"
#include "window.h"

int gasave = 256 ;		/* global ASAVE size            */
int gacount = 256 ;		/* count until next ASAVE       */


/* insert a # into the text here...we are in CMODE */
static int inspound( int n) {
/* if we are at the beginning of the line, no go */
	if( n == 1 && curwp->w_doto != 0) {
		int i ;

	/* scan to see if all space before this is white space */
		for( i = curwp->w_doto - 1 ; i >= 0 ; i -= 1) {
			int ch ;	/* last character before input */

			ch = lgetc( curwp->w_dotp, i) ;
			if( ch != ' ' && ch != '\t')
				break ;
		}

	/* delete back first */
		if( i < 0)
			while( getccol( FALSE) >= 1)
				backdel( FALSE, 1) ;
	}

/* and insert the required pound */
	return linsert( n, '#') ;
}

/*
 * insert a brace into the text here...we are in CMODE
 *
 * int n;	repeat count
 * int c;	brace to insert (if not }, just normal insertion).
 */
static int insbrace( int n, int c) {
	int ch ;			/* last character before input */
	int oc ;			/* caractere oppose a c */
	int i, count ;
	int target ;		/* column brace should go after */
	struct line *oldlp ;
	int oldoff ;

/* if not called with {, acts as insertion */
	if( c == '}')
		oc = '{' ;
	else
		return linsert( n, c) ;

/* scan to see if all preceding spaces are white spaces, if not, insert */
	for( i = curwp->w_doto - 1 ; i >= 0 ; --i) {
		ch = lgetc( curwp->w_dotp, i) ;
		if( ch != ' ' && ch != '\t')
			return linsert( n, c) ;
	}

	oldlp = curwp->w_dotp ;
	oldoff = curwp->w_doto ;

	count = 1 ;
	do {
		if( boundry( curwp->w_dotp, curwp->w_doto, REVERSE)) {
		/* at beginning of buffer, no match to be found */
			curwp->w_dotp = oldlp ;
			curwp->w_doto = oldoff ;
			return linsert( n, c) ;
		}
		
		backchar( FALSE, 1) ;

		/* if not eol */
		if( curwp->w_doto != llength( curwp->w_dotp)) {
			ch = lgetc( curwp->w_dotp, curwp->w_doto) ;
			if( ch == c)
				++count ;
			else if( ch == oc)
				--count ;
		}
	} while( count > 0) ;

	curwp->w_doto = 0 ;	/* debut de ligne */
	/* aller au debut de la ligne apres la tabulation */
	while( (ch = lgetc( curwp->w_dotp, curwp->w_doto)) == ' '
	       || ch == '\t')
		forwchar( FALSE, 1) ;

	/* delete back first */
	target = getccol( FALSE) ;	/* c'est l'indent que l'on doit avoir */
	curwp->w_dotp = oldlp ;
	curwp->w_doto = oldoff ;

	while( target != getccol( FALSE)) {
		if( target < getccol( FALSE))	/* on doit detruire des caracteres */
			while( getccol( FALSE) > target)
				backdel( FALSE, 1) ;
		else {		/* on doit en inserer */
			while( target - getccol( FALSE) >= tabwidth)
				insert_tab( FALSE, 1) ;

			linsert( target - getccol( FALSE), ' ') ;
		}
	}

	/* and insert the required brace(s) */
	return linsert( n, c) ;
}

#if CFENCE
/*
 * Close fences are matched against their partners, and if
 * on screen the cursor briefly lights there
 *
 * char ch;			fence type to match against
 */
static void fmatch( int ch) {
	struct line *oldlp ;	/* original line pointer */
	int oldoff ;			/* and offset */
	struct line *toplp ;	/* top line in current window */
	int count ;				/* current fence level count */
	int opench ;			/* open fence */

	/* $tpause <= 0 disable fmatch */
	if( term.t_pause <= 0)
		return ;

	/* first get the display update out there */
	update( FALSE) ;

	/* save the original cursor position */
	oldlp = curwp->w_dotp ;
	oldoff = curwp->w_doto ;

	/* setup proper open fence for passed close fence */
	if( ch == ')')
		opench = '(' ;
	else if( ch == '}')
		opench = '{' ;
	else
		opench = '[' ;

	/* find the top line and set up for scan */
	toplp = curwp->w_linep->l_bp ;
	backchar( FALSE, 1) ;	/* . was after the }, move back */

	/* scan back until we find it, or reach past the top of the window */
	count = 1 ;
	do {
		/* At beginning of window or buffer, no match to be found */
		if( curwp->w_dotp == toplp
		||	boundry( curwp->w_dotp, curwp->w_doto, REVERSE))
			break ;

		backchar( FALSE, 1) ;

		/* if not eol */
		if( curwp->w_doto != llength(curwp->w_dotp)) {
			int c ;					/* current character in scan */

			c = lgetc( curwp->w_dotp, curwp->w_doto) ;
			if( c == ch)
				++count ;
			else if( c == opench)
				--count ;
		}
	} while( count > 0) ;

	/* if count is zero, we have a match, display the sucker */
	if( count == 0) {
		int i ;

		/* there is a real machine dependant timing problem here we have
		   yet to solve......... */
		for( i = 0 ; i < term.t_pause ; i++) {
			update( FALSE) ;
			usleep( 10000L) ;
		}
	}

	/* restore the current position */
	curwp->w_dotp = oldlp ;
	curwp->w_doto = oldoff ;
}
#endif


/*
 * This is the general command execution routine. It handles the fake binding
 * of all the keys to "self-insert". It also clears out the "thisflag" word,
 * and arranges to move it to the "lastflag", so that the next command can
 * look at it. Return the status of command.
 */
int execute( int c, int f, int n) {
	int status ;

/* if the keystroke is a bound function...do it */
	fnp_t execfunc = getbind( c) ;
	if( execfunc != NULL) {
		thisflag = 0 ;
		const name_bind *nbp = getnamebind( execfunc) ;
		if( nbp->tag && curbp->b_mode & MDVIEW)
			status = rdonly() ;
		else
			status = execfunc( f, n) ;

		lastflag = thisflag ;
		return status ;
	}

/* non insertable character can only be bound to function */
	if( c < 0x20
	||	(c >= 0x7F && c < 0xA0)
	||  c > 0x10FFFF) {	/* last valid unicode */
		lastflag = 0 ;						/* Fake last flags. */
		mloutfmt( "%B(Key not bound)") ;	/* Complain */
		return FALSE ;
	}

/* insertable character => self insert, check if buffer is read only */
	if( curbp->b_mode & MDVIEW) {
		lastflag = 0 ;
		return rdonly() ;
	}

/* check valid count */
	if( n <= 0) {
		lastflag = 0 ;
		return n < 0 ? FALSE : TRUE ;
	}

/* wrap on space after fill column in wrap mode */
	if( c == ' '
	&&	(curwp->w_bufp->b_mode & MDWRAP)
	&&	fillcol > 0
	&&	getccol( FALSE) > fillcol) {
		status = execute( META | SPEC | 'W', FALSE, 1) ; /* defaults to wrapword */
		if( status != TRUE) {
			lastflag = 0 ;
			return status ;
		}
	}

	thisflag = 0 ;	/* For the future.      */

/* following handling of overwrite is only valid when n == 1 */
	/* if we are in overwrite mode, not at eol,
	   and next char is not a tab or we are at a tab stop,
	   delete a char forward								*/
	if( curbp->b_mode & MDOVER
	&&	curwp->w_doto < curwp->w_dotp->l_used
	&&	(lgetc( curwp->w_dotp, curwp->w_doto) != '\t' ||
		((curwp->w_doto) % tabwidth) == (tabwidth - 1)))
		ldelchar( 1, FALSE) ;

/* do the appropriate insertion */
	switch( c) {
	case '}':
	case ']':
	case ')':
	case '#':
		if( (curbp->b_mode & MDCMOD) != 0) {
			if( c == '#')
				status = inspound( n) ;
			else {
				status = insbrace( n, c) ;
#if	CFENCE
				if( status == TRUE)
					fmatch( c) ;	/* check for CMODE fence matching */
#endif
			}

			break ;
		}

		/* fallthrough */
	default:
		status = linsert( n, c) ;
	}

/* perform auto-save */
	if( status == TRUE				/* successful insertion */
	&&	(curbp->b_mode & MDASAVE)	/* auto save is on */
	&&	(--gacount == 0)) {			/* insertion count reached */
		/* and save the file if needed */
		upscreen( FALSE, 0) ;
		filesave( FALSE, 0) ;
		gacount = gasave ;
	}

	lastflag = thisflag ;
	return status ;
}


void kbd_loop( void) {
	int c = -1 ;	/* command character */

/* Setup to process commands. */
	lastflag = 0 ;  /* Fake last flags. */

  for( ;;) {
	int saveflag ;	/* temp store for lastflag */
	int basec ;		/* c stripped of meta character */
	int f ;			/* default flag */
	int n ;			/* numeric repeat count */

	/* Execute the "command" macro...normally null. */
	saveflag = lastflag ;	/* Preserve lastflag through this. */
	execute( META | SPEC | 'C', FALSE, 1) ;
	lastflag = saveflag ;

#if TYPEAH && PKCODE
	if( typahead()) {
		int newc ;

		newc = getcmd() ;
		update( FALSE) ;
		do {
			fnp_t execfunc ;

			if( c == newc
			&& (execfunc = getbind( c)) != NULL
			&& execfunc != insert_newline
			&& execfunc != insert_tab)
				newc = getcmd() ;
			else
				break ;
		} while( typahead()) ;
		c = newc ;
	} else {
		update( FALSE) ;
		c = getcmd() ;
	}
#else
	/* Fix up the screen    */
	update( FALSE) ;

	/* get the next command from the keyboard */
	c = getcmd() ;
#endif
	/* if there is something on the command line, clear it */
	if( mpresf != FALSE) {
		mloutstr( "") ;
		update( FALSE) ;
#if	CLRMSG
		if( c == ' ')	/* ITS EMACS does this  */
			continue ;
#endif
	}

	f = FALSE ;
	n = 1 ;

	/* do META-# processing if needed */
	/* do ^U repeat argument processing */
	while( c == reptc
	||( (c & META)
		&&	(((basec = c & ~META) >= '0' && basec <= '9') || basec == '-'))) {
		int mflag = 0 ;		/* minus flag, default to positive */

		f = TRUE ;
		if( c == reptc) {
			n = 4 ;
			basec = 2 ;			/* lead by universal arg cmd */
		} else if( c & META) {
			if( basec == '-') {
				mflag = TRUE ;	/* negative */
				n = 1 ;
				basec = 1 ;		/* lead by M-- */
			} else {
				n = basec - '0' ;
				basec = 0 ;		/* lead by M-# */
			}
		}

		mloutfmt( "Arg: %s%d", mflag ? "-" : "", n) ;
		while( ((c = getcmd()) >= '0' && c <= '9') || c == '-') {
			if( c == '-') {
				if( basec == 2) {	/* directly follows universal arg cmd */
					n = 1 ;
					basec = 1 ;
					mflag = TRUE ;
				} else
					break ;
			} else {
				if( basec) { /* follows universal arg cmd or leading dash */
					n = c - '0' ;
					basec = 0 ;
				} else if( n <= 0xCCCCCCB)	/* avoid overflow */
					n = n * 10 + c - '0' ;
			}

			mloutfmt( "Arg: %s%d", mflag ? "-" : "", n) ;
		}

		if( mflag)
			n = -n ;
	}

	/* and execute the command */
	execute( c, f, n) ;
  }
}


/* end of execute.c */
