/* names.c -- implements names.h */
#include "names.h"

/* Name to function binding table.
 *
 * This table gives the names of all the bindable functions and their C
 * function address.  These are used for the bind-to-key function and
 * command line parsing.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "basic.h"
#include "bind.h"
#include "bindable.h"
#include "buffer.h"
#include "display.h"
#include "eval.h"
#include "exec.h"
#include "file.h"
#include "isearch.h"
#include "line.h"
#include "region.h"
#include "random.h"
#include "search.h"
#include "spawn.h"
#include "util.h"
#include "window.h"
#include "word.h"


#define CTL_ CONTROL

const name_bind names[] = {
	{" abort-command", ctrlg,	 					CTL_ | 'G'} ,
	{" add-global-mode", setgmode,					META | 'M'} ,
	{" add-mode", setemode,							CTLX | 'M'} ,
#if	APROP
	{" apropos", apro,								META | 'A'} ,
#endif
	{" backward-character", (fnp_t) backchar,		CTL_ | 'B'} ,
	{" begin-macro", ctlxlp,						CTLX | '('} ,
	{" beginning-of-file", (fnp_t) gotobob,			META | '<'} ,
	{" beginning-of-line", (fnp_t) gotobol,			CTL_ | 'A'} ,
	{" bind-to-key", bindtokey,						META | 'K'} ,
	{" buffer-position", showcpos,					CTLX | '='} ,
	{"!case-region-lower", lowerregion,				CTLX | CTL_ | 'L'} ,
	{"!case-region-upper", upperregion,				CTLX | CTL_ | 'U'} ,
	{"!case-word-capitalize", capword,				META | 'C'} ,
	{"!case-word-lower", lowerword,					META | 'L'} ,
	{"!case-word-upper", upperword,					META | 'U'} ,
	{" change-file-name", filename,					CTLX | 'N'} ,
	{" change-screen-size", newsize,				META | CTL_ | 'D'} , /* M^S */
	{" change-screen-width", newwidth,				META | CTL_ | 'T'} ,
	{" clear-and-redraw", redraw, 					CTL_ | 'L'} ,
	{" clear-message-line", clrmes, 0} ,
	{" copy-region", copyregion,					META | 'W'} ,
#if	WORDPRO
	{" count-words", wordcount,						META | CTL_ | 'C'} ,
#endif
	{" ctlx-prefix", cex, 							CTL_ | 'X'} ,
	{"!delete-blank-lines", deblank,				CTLX | CTL_ | 'O'} ,
	{" delete-buffer", killbuffer,					CTLX | 'K'} ,
	{" delete-global-mode", delgmode,				META | CTL_ | 'M'} ,
	{" delete-mode", delmode,						CTLX | CTL_ | 'M'} ,
	{"!delete-next-character", forwdel,				CTL_ | 'D'} ,
	{"!delete-next-word", delfword,					META | 'D'} ,
	{" delete-other-windows", onlywind,				CTLX | '1'} ,
	{"!delete-previous-character", backdel,			CTL_ | 'H'} , /* ^? */
	{"!delete-previous-word", delbword,				META | CTL_ | 'H'} , /* M^? */
	{" delete-window", delwind,						CTLX | '0'} ,
	{" describe-bindings", desbind, 0} ,
	{" describe-key", deskey,						CTLX | '?'} ,
#if	AEDIT
	{"!detab-line", detab, 							CTLX | CTL_ | 'D'} , /* X^A */
#endif
	{" end-macro", ctlxrp,							CTLX | ')'} ,
	{" end-of-file", (fnp_t) gotoeob,				META | '>'} ,
	{" end-of-line", (fnp_t) gotoeol,				CTL_ | 'E'} ,
#if	AEDIT
	{"!entab-line", entab, 							CTLX | CTL_ | 'E'} ,
#endif
	{" exchange-point-and-mark", (fnp_t) swapmark,	CTLX | CTL_ | 'X'} ,
	{" execute-buffer", execbuf, 0} ,
	{" execute-command-line", execcmd, 0} ,
	{" execute-file", execfile, 0} ,
	{" execute-macro", ctlxe,						CTLX | 'E'} ,
	{" execute-macro-1", cbuf1, 0} ,
	{" execute-macro-10", cbuf10, 0} ,
	{" execute-macro-11", cbuf11, 0} ,
	{" execute-macro-12", cbuf12, 0} ,
	{" execute-macro-13", cbuf13, 0} ,
	{" execute-macro-14", cbuf14, 0} ,
	{" execute-macro-15", cbuf15, 0} ,
	{" execute-macro-16", cbuf16, 0} ,
	{" execute-macro-17", cbuf17, 0} ,
	{" execute-macro-18", cbuf18, 0} ,
	{" execute-macro-19", cbuf19, 0} ,
	{" execute-macro-2", cbuf2, 0} ,
	{" execute-macro-20", cbuf20, 0} ,
	{" execute-macro-21", cbuf21, 0} ,
	{" execute-macro-22", cbuf22, 0} ,
	{" execute-macro-23", cbuf23, 0} ,
	{" execute-macro-24", cbuf24, 0} ,
	{" execute-macro-25", cbuf25, 0} ,
	{" execute-macro-26", cbuf26, 0} ,
	{" execute-macro-27", cbuf27, 0} ,
	{" execute-macro-28", cbuf28, 0} ,
	{" execute-macro-29", cbuf29, 0} ,
	{" execute-macro-3", cbuf3, 0} ,
	{" execute-macro-30", cbuf30, 0} ,
	{" execute-macro-31", cbuf31, 0} ,
	{" execute-macro-32", cbuf32, 0} ,
	{" execute-macro-33", cbuf33, 0} ,
	{" execute-macro-34", cbuf34, 0} ,
	{" execute-macro-35", cbuf35, 0} ,
	{" execute-macro-36", cbuf36, 0} ,
	{" execute-macro-37", cbuf37, 0} ,
	{" execute-macro-38", cbuf38, 0} ,
	{" execute-macro-39", cbuf39, 0} ,
	{" execute-macro-4", cbuf4, 0} ,
	{" execute-macro-40", cbuf40, 0} ,
	{" execute-macro-5", cbuf5, 0} ,
	{" execute-macro-6", cbuf6, 0} ,
	{" execute-macro-7", cbuf7, 0} ,
	{" execute-macro-8", cbuf8, 0} ,
	{" execute-macro-9", cbuf9, 0} ,
	{" execute-named-command", namedcmd,		META | 'X'} ,
#if	PROC
	{" execute-procedure", execproc,			META | CTL_ | 'E'} ,
#endif
	{" execute-program", execprg,				CTLX | '$'} ,
	{" exit-emacs", quit, 						CTLX | CTL_ | 'C'} ,
#if	WORDPRO
	{"!fill-paragraph", fillpara,				META | 'Q'} ,
#endif
	{"!filter-buffer", filter_buffer,			CTLX | '#'} ,
	{" find-file", filefind, 					CTLX | CTL_ | 'F'} ,
	{" forward-character", (fnp_t) forwchar,	CTL_ | 'F'} ,
	{" goto-line", gotoline,					META | 'G'} ,
#if	CFENCE
	{" goto-matching-fence", getfence,			META | CTL_ | 'F'} ,
#endif
	{" grow-window", enlargewind,				CTLX | 'Z'} , /* X^ */
	{"!handle-tab", insert_tab,					CTL_ | 'I'} ,
	{" help", help,								META | '?'} ,
	{" hunt-backward", backhunt, 0} ,
	{" hunt-forward", forwhunt,					META | 'S'} ,
	{" i-shell", spawncli,						CTLX | 'C'} ,
#if	ISRCH
	{" incremental-search", fisearch,			CTLX | 'S'} ,
#endif
	{"!insert-file", insfile,					CTLX | CTL_ | 'I'} ,
	{"!insert-space", insspace,					CTL_ | 'C'} ,
	{"!insert-string", istring, 0} ,
#if	WORDPRO
#if	PKCODE
	{"!justify-paragraph", justpara,			META | 'J'} ,
#endif
	{"!kill-paragraph", killpara,				META | CTL_ | 'W'} ,
#endif
	{"!kill-region", killregion, 				CTL_ | 'W'} ,
	{"!kill-to-end-of-line", killtext, 			CTL_ | 'K'} ,
	{" list-buffers", listbuffers, 				CTLX | CTL_ | 'B'} ,
	{" meta-prefix", metafn, 					CTL_ | '['} ,
	{" move-window-down", mvdnwind,				CTLX | CTL_ | 'N'} ,
	{" move-window-up", mvupwind,				CTLX | CTL_ | 'P'} ,
	{" name-buffer", namebuffer,				META | CTL_ | 'N'} ,
	{"!newline", insert_newline, 				CTL_ | 'M'} ,
	{"!newline-and-indent", indent, 			CTL_ | 'J'} ,
	{" next-buffer", nextbuffer,				CTLX | 'X'} ,
	{" next-line", (fnp_t) forwline, 			CTL_ | 'N'} ,
	{" next-page", (fnp_t) forwpage,			CTL_ | 'V'} ,
#if	WORDPRO
	{" next-paragraph", gotoeop,				META | 'N'} ,
#endif
	{" next-window", nextwind,					CTLX | 'O'} ,
	{" next-word", forwword,					META | 'F'} ,
	{" nop", nullproc,							SPEC | META | 'C'}, /* hook */
	{"!open-line", openline,					CTL_ | 'O'} ,
	{" overwrite-string", ovstring, 0} ,
	{" pipe-command", pipecmd,					CTLX | '@'} ,
	{" previous-line", (fnp_t) backline, 		CTL_ | 'P'} ,
	{" previous-page", (fnp_t) backpage,		CTL_ | 'Z'} , /* MV */
#if	WORDPRO
	{" previous-paragraph", gotobop,			META | 'P'} ,
#endif
	{" previous-window", prevwind,				CTLX | 'P'} ,
	{" previous-word", backword,				META | 'B'} ,
	{"!query-replace-string", qreplace,			META | CTL_ | 'R'} ,
	{" quick-exit", quickexit,					META | 'Z'} ,
	{"!quote-character", quote, 				CTL_ | 'Q'} , /* also XQ */
	{"!read-file", fileread,					CTLX | CTL_ | 'R'} ,
	{" redraw-display", reposition,				META | CTL_ | 'L'} , /* M! */
	{"!replace-string", sreplace,				META | 'R'} ,
	{" resize-window", resize,					CTLX | 'W'} ,
	{" restore-window", restwnd, 0} ,
#if	ISRCH
	{" reverse-incremental-search", risearch,	CTLX | 'R'} ,
#endif
#if	PROC
	{" run", execproc, 0} ,	// alias of execute-procedure
#endif
	{"!save-file", filesave,					CTLX | CTL_ | 'S'} , /* also X^D */
	{" save-window", savewnd, 0} ,
	{" scroll-next-down", scrnextdw,			META | CTL_ | 'V'} ,
	{" scroll-next-up", scrnextup,				META | CTL_ | 'Z'} ,
	{" search-forward", forwsearch, 			CTL_ | 'S'} ,
	{" search-reverse", backsearch, 			CTL_ | 'R'} ,
	{" select-buffer", usebuffer,				CTLX | 'B'} ,
	{" set", setvar,							CTLX | 'A'} ,
	{" set-fill-column", setfillcol,			CTLX | 'F'} ,
	{" set-mark", (fnp_t) setmark,				META | ' '} , /* M. */
	{" shell-command", spawn,					CTLX | '!'} ,
	{" shrink-window", shrinkwind,				CTLX | CTL_ | 'Z'} ,
	{" split-current-window", splitwind,		CTLX | '2'} ,
	{" store-macro", storemac, 0} ,
#if	PROC
	{" store-procedure", storeproc, 0} ,
#endif
#if	BSD | SVR4
	{" suspend-emacs", bktoshell,				CTLX | 'D'} , /* BSD MS */
#endif
	{"!transpose-characters", (fnp_t) twiddle,	CTL_ | 'T'} ,
#if	AEDIT
	{"!trim-line", trim,						CTLX | CTL_ | 'T'} ,
#endif
	{" unbind-key", unbindkey,					META | CTL_ | 'K'} ,
	{" universal-argument", unarg, 				CTL_ | 'U'} ,
	{" unmark-buffer", unmark,					META | '~'} ,
	{" update-screen", upscreen, 0} ,
	{" view-file", viewfile, 					CTLX | CTL_ | 'V'} ,
	{"!wrap-word", wrapword,					SPEC | META | 'W'} , /* hook */
	{" write-file", filewrite,					CTLX | CTL_ | 'W'} ,
	{" write-message", writemsg, 0} ,
	{"!yank", yank,								CTL_ | 'Y'} ,

	{" ", NULL, 0},
/* extra key mapping */
//	{ NULL, newsize,							META | CTL_ | 'S'},
	{ NULL, backdel,							CTL_ | '?'},
	{ NULL, delbword,							META | CTL_ | '?'},
	{ NULL, detab,								CTLX | CTL_ | 'A'},
	{ NULL, enlargewind,						CTLX | '^'},
	{ NULL, (fnp_t) backpage,					META | 'V'},
	{ NULL, quote,				 				CTLX | 'Q'},
	{ NULL, reposition,							META | '!'},
//detab { NULL, filesave,						CTLX | CTL_ | 'D'},
	{ NULL, (fnp_t) setmark,					META | '.'},
//	{ NULL, bktoshell,							META | 'S'},

#if VT220
	{ NULL, yank,								SPEC | '2'}, /* Insert */
	{ NULL, forwdel /* killregion */,			SPEC | '3'}, /* Delete */
    { NULL, (fnp_t) backpage,					SPEC | '5'}, /* Page Up */
    { NULL, (fnp_t) forwpage,					SPEC | '6'}, /* Page Down */
    { NULL, (fnp_t) backline,					SPEC | 'A'}, /* Up */
    { NULL, (fnp_t) forwline,					SPEC | 'B'}, /* Down */
    { NULL, (fnp_t) forwchar,					SPEC | 'C'}, /* Right */
    { NULL, (fnp_t) backchar,					SPEC | 'D'}, /* Left */
    { NULL, (fnp_t) gotoeob,					SPEC | 'F'}, /* End */
    { NULL, (fnp_t) gotobob,					SPEC | 'H'}, /* Home */
    { NULL, help,								SPEC | 'P'}, /* F1 */
#endif

/* hooks */
	{ NULL, nullproc,							SPEC | META | 'R'}, /* hook */
	{ NULL, nullproc,							SPEC | META | 'X'}, /* hook */

	{ NULL, NULL, 0}
} ;

static int lastnmidx = 0 ;	/* index of last name entry */


key_tab *keytab ;
static int ktsize = 140 ;	/* last check: need at least 133 + 1 */


boolean init_bindings( void) {
/* allocate table */
	keytab = malloc( ktsize * sizeof *keytab) ;
	if( keytab == NULL)
		return FALSE ;

/* insert end of table mark */
	keytab->k_code = 0 ;
	keytab->k_nbp = NULL ;

/* Add default key bindings */
	const name_bind *nbp ;
	for( nbp = names ; nbp->n_func != NULL ; nbp++) {
	/* Check entries and strict order */
		assert( (nbp->n_name != NULL) &&
				((nbp == names) ||
							(0 > strcmp( bind_name( nbp - 1), bind_name( nbp)))
				)
		) ;

	/* Add key definition */
		if( nbp->n_keycode) {
			key_tab *ktp = setkeybinding( nbp->n_keycode, nbp) ;
		/* check it was indeed an insertion at end of table not a
		 * key code re-definition */
			assert( (++ktp)->k_code == 0) ;
		}
	}

/* memorize position after last valid function entry */
	lastnmidx = nbp - names ;

/* Process extra key bindings if any */
	for( nbp++ ; nbp->n_func != NULL ; nbp++) {
	/* Check entry */
		assert( nbp->n_keycode && (nbp->n_name == NULL)) ;

	/* Look for corresponding function and add extra key binding */
		const name_bind *fnbp ;
		for( fnbp = names ; fnbp->n_func != NULL ; fnbp++)
			if( fnbp->n_func == nbp->n_func) {
				setkeybinding( nbp->n_keycode, fnbp) ;
				break ;
			}

	/* Insure there is a name entry for the keycode */
		assert( fnbp->n_func != NULL) ;
	}

	return TRUE ;
}

key_tab *setkeybinding( unsigned key, const name_bind *nbp) {
	key_tab *ktp ;

/* search the table to see if it exists */
    for( ktp = keytab ; ktp->k_code != 0 ; ktp++)
        if( ktp->k_code == key) {
		/* it exists, just change it then */
			ktp->k_nbp = nbp ;
			return ktp ;
        }

/* otherwise we need to add it to the end */
	/* check if the end marker is at the end of the table */
    if( ktp == &keytab[ ktsize - 1]) {
    /* out of binding room */
		int newsize = ktsize + 10 ;
		key_tab *newkeytab = realloc( keytab, newsize * sizeof *keytab) ;
		if( newkeytab == NULL)
		/* out of space */
	        return ktp ;

		keytab = newkeytab ;
		ktp = &keytab[ ktsize - 1] ;
		ktsize = newsize ;
	}

    ktp->k_code = key ;		/* add keycode */
	ktp->k_nbp = nbp ;
    ++ktp ;      			/* and make sure the next is null */
    ktp->k_code = 0 ;
	ktp->k_nbp = NULL ;
    return ktp - 1 ;
}

boolean delkeybinding( unsigned key) {
    key_tab *ktp ;   /* pointer into the key binding table */

/* search the table to see if the key exists */
    for( ktp = keytab ; ktp->k_code != 0 ; ktp++) {
        if( ktp->k_code == key) {
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

#define BINARY 1

const name_bind *fncmatch( char *name) {
#ifdef BINARY
	int found = lastnmidx ;
	int low = 0 ;
	int high = found - 1 ;
	do {
		int cur = (high + low) / 2 ;
		int s = strcmp( name, bind_name( &names[ cur])) ;
		if( s < 0)
			high = cur - 1 ;
		else if( s == 0) {
			found = cur ;
			break ;
		} else
			low = cur + 1 ;
	} while( low <= high) ;

	return &names[ found] ;
#else
	const name_bind *nbp ;
	for( nbp = names ; nbp->n_func != NULL ; nbp++)
		if( !strcmp( name, bind_name( nbp)))
			break ;

	return nbp ;
#endif
}


/* user function that does NOTHING */
BINDABLE( nullproc) {
	return TRUE ;
}

/* dummy function for binding to meta prefix */
BINDABLE( metafn) {
	return TRUE ;
}

/* dummy function for binding to control-x prefix */
BINDABLE( cex) {
	return TRUE ;
}

/* dummy function for binding to universal-argument */
BINDABLE( unarg) {
	return TRUE ;
}

/* end of names.c */
