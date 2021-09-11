/* eval.c -- implements eval.h */
#include "eval.h"

/*	Expression evaluation functions
 *
 *	written 1986 by Daniel Lawrence
 *	modified by Petri Kutvonen
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "basic.h"
#include "bind.h"
#include "buffer.h"
#include "defines.h"
#include "display.h"
#include "exec.h"
#include "execute.h"
#include "flook.h"
#include "input.h"
#include "line.h"
#include "random.h"
#include "search.h"
#include "terminal.h"
#include "termio.h"
#include "util.h"
#include "version.h"
#include "window.h"

/*	Macro argument token types					*/

#define	TKNUL	0		/* end-of-string                */
#define	TKARG	1		/* interactive argument         */
#define	TKBUF	2		/* buffer argument              */
#define	TKVAR	3		/* user variables               */
#define	TKENV	4		/* environment variables        */
#define	TKFUN	5		/* function....                 */
#define	TKDIR	6		/* directive                    */
#define	TKLBL	7		/* line label                   */
#define	TKLIT	8		/* numeric literal              */
#define	TKSTR	9		/* quoted string literal        */
#define	TKCMD	10		/* command name                 */


/* Emacs global flag bit definitions (for gflags). */
/* if GFREAD is set, current buffer will be set on first file (read in) */
#define	GFREAD	1

static int gflags = GFREAD ;	/* global control flag		*/

int readfirst_f( void) {
	return GFREAD == (gflags & GFREAD) ;
}

int macbug = FALSE ;		/* macro debuging flag          */
int cmdstatus = TRUE ;		/* last command status          */
static int flickcode = FALSE ;		/* do flicker supression?       */
int rval = 0 ;			/* return value of a subprocess */


static int saveflag = 0 ;	/* Flags, saved with the $target var */


unsigned envram = 0 ;		/* # of bytes current in use by malloc */


/* User variables ******************************************/
#define	MAXVARS	255
#define	NVSIZE	10	/* Max #chars in a var name. */

/* Structure to hold user variables and their definitions. */
static struct {
	char u_name[ NVSIZE + 1] ;	/* name of user variable */
	char *u_value ;				/* value (string) */
} uv[ MAXVARS + 1] ;


static char errorm[] = "ERROR" ;	/* error literal                */

static int seed = 0 ;			/* random number seed           */

static char *ltos( int val) ;
static char *mkupper( char *dst, char *src) ;

/* List of recognized environment variables. */

static const char *envars[] = {
	"fillcol",		/* current fill column */
	"pagelen",		/* number of lines used by editor */
	"curcol",		/* current column pos of cursor */
	"curline",		/* current line in file */
	"ram",			/* ram in use by malloc */
	"flicker",		/* flicker supression */
	"curwidth",		/* current screen width */
	"cbufname",		/* current buffer name */
	"cfname",		/* current file name */
	"sres",			/* current screen resolution */
	"debug",		/* macro debugging */
	"status",		/* returns the status of the last command */
	"palette",		/* current palette string */
	"asave",		/* # of chars between auto-saves */
	"acount",		/* # of chars until next auto-save */
	"lastkey",		/* last keyboard char struck */
	"curchar",		/* current character under the cursor */
	"discmd",		/* display commands on command line */
	"version",		/* current version number */
	"progname",		/* returns current prog name - "MicroEMACS" */
	"seed",			/* current random number seed */
	"disinp",		/* display command line input characters */
	"wline",		/* # of lines in current window */
	"cwline",		/* current screen line in window */
	"target",		/* target for line moves */
	"search",		/* search pattern */
	"replace",		/* replacement pattern */
	"match",		/* last matched magic pattern */
	"kill",			/* kill buffer (read only) */
	"cmode",		/* mode of current buffer */
	"gmode",		/* global modes */
	"tpause",		/* length to pause for paren matching */
	"pending",		/* type ahead pending flag */
	"lwidth",		/* width of current line */
	"line",			/* text of current line */
	"gflags",		/* global internal emacs flags */
	"rval",			/* child process return value */
	"tab",			/* tab width, 1... */
	"hardtab",		/* TRUE for hard coded tab, FALSE for soft ones */
	"viewtab",		/* TRUE to visualize hard coded tabs */
	"overlap",
	"jump",
#if SCROLLCODE
	"scroll",		/* scroll enabled */
#endif
};

/* And its preprocessor definitions. */

#define	EVFILLCOL	0
#define	EVPAGELEN	1
#define	EVCURCOL	2
#define	EVCURLINE	3
#define	EVRAM		4
#define	EVFLICKER	5
#define	EVCURWIDTH	6
#define	EVCBUFNAME	7
#define	EVCFNAME	8
#define	EVSRES		9
#define	EVDEBUG		10
#define	EVSTATUS	11
#define	EVPALETTE	12
#define	EVASAVE		13
#define	EVACOUNT	14
#define	EVLASTKEY	15
#define	EVCURCHAR	16
#define	EVDISCMD	17
#define	EVVERSION	18
#define	EVPROGNAME	19
#define	EVSEED		20
#define	EVDISINP	21
#define	EVWLINE		22
#define EVCWLINE	23
#define	EVTARGET	24
#define	EVSEARCH	25
#define	EVREPLACE	26
#define	EVMATCH		27
#define	EVKILL		28
#define	EVCMODE		29
#define	EVGMODE		30
#define	EVTPAUSE	31
#define	EVPENDING	32
#define	EVLWIDTH	33
#define	EVLINE		34
#define	EVGFLAGS	35
#define	EVRVAL		36
#define EVTAB		37
#define EVHARDTAB	38
#define EVVIEWTAB	39
#define EVOVERLAP	40
#define EVSCROLLCOUNT	41
#define EVSCROLL	42

enum function_type {
	NILNAMIC	= 0,
	MONAMIC		= (1 << 6),
	DYNAMIC		= (2 << 6),
	TRINAMIC	= (3 << 6)
} ;

#define ARGCOUNT( ft) (ft >> 6)
#define FUNID( ft) (ft & ((1 << 6) - 1))

enum function_code {
	UFADD = 0,	UFSUB,		UFTIMES,	UFDIV,		UFMOD,		UFNEG,
	UFCAT,		UFLEFT,		UFRIGHT,	UFMID,		UFNOT,		UFEQUAL,
	UFLESS,		UFGREATER,	UFSEQUAL,	UFSLESS,	UFSGREAT,	UFIND,
	UFAND,		UFOR,		UFLENGTH,	UFUPPER,	UFLOWER,	UFTRUTH,
	UFASCII,	UFCHR,		UFGTKEY,	UFRND,		UFABS,		UFSINDEX,
	UFENV,		UFBIND,		UFEXIST,	UFFIND,		UFBAND,		UFBOR,
	UFBXOR,		UFBNOT,		UFXLATE
} ;

/* List of recognized user functions. */
static struct {
	const char f_name[ 4] ;
	const int  f_type ;
} const funcs[] = {
	{ "abs", UFABS		| MONAMIC },	/* absolute value of a number */
	{ "add", UFADD		| DYNAMIC },	/* add two numbers together */
	{ "and", UFAND		| DYNAMIC },	/* logical and */
	{ "asc", UFASCII	| MONAMIC },	/* char to integer conversion */
	{ "ban", UFBAND		| DYNAMIC },	/* bitwise and   9-10-87  jwm */
	{ "bin", UFBIND		| MONAMIC },	/* look up function name bound to key */
	{ "bno", UFBNOT		| MONAMIC },	/* bitwise not */
	{ "bor", UFBOR		| DYNAMIC },	/* bitwise or    9-10-87  jwm */
	{ "bxo", UFBXOR		| DYNAMIC },	/* bitwise xor   9-10-87  jwm */
	{ "cat", UFCAT		| DYNAMIC },	/* concatenate string */
	{ "chr", UFCHR		| MONAMIC },	/* integer to char conversion */
	{ "div", UFDIV		| DYNAMIC },	/* division */
	{ "env", UFENV		| MONAMIC },	/* retrieve a system environment var */
	{ "equ", UFEQUAL	| DYNAMIC },	/* logical equality check */
	{ "exi", UFEXIST	| MONAMIC },	/* check if a file exists */
	{ "fin", UFFIND		| MONAMIC },	/* look for a file on the path... */
	{ "gre", UFGREATER	| DYNAMIC },	/* logical greater than */
	{ "gtk", UFGTKEY	| NILNAMIC },	/* get 1 charater */
	{ "ind", UFIND		| MONAMIC },	/* evaluate indirect value */
	{ "lef", UFLEFT		| DYNAMIC },	/* left string(string, len) */
	{ "len", UFLENGTH	| MONAMIC },	/* string length */
	{ "les", UFLESS		| DYNAMIC },	/* logical less than */
	{ "low", UFLOWER	| MONAMIC },	/* lower case string */
	{ "mid", UFMID		| TRINAMIC },	/* mid string(string, pos, len) */
	{ "mod", UFMOD		| DYNAMIC },	/* mod */
	{ "neg", UFNEG		| MONAMIC },	/* negate */
	{ "not", UFNOT		| MONAMIC },	/* logical not */
	{ "or",  UFOR		| DYNAMIC },	/* logical or */
	{ "rig", UFRIGHT	| DYNAMIC },	/* right string(string, pos) */
	{ "rnd", UFRND		| MONAMIC },	/* get a random number */
	{ "seq", UFSEQUAL	| DYNAMIC },	/* string logical equality check */
	{ "sgr", UFSGREAT	| DYNAMIC },	/* string logical greater than */
	{ "sin", UFSINDEX	| DYNAMIC },	/* find the index of one string in another */
	{ "sle", UFSLESS	| DYNAMIC },	/* string logical less than */
	{ "sub", UFSUB		| DYNAMIC },	/* substraction */
	{ "tim", UFTIMES	| DYNAMIC },	/* multiplication */
	{ "tru", UFTRUTH	| MONAMIC },	/* Truth of the universe logical test */
	{ "upp", UFUPPER	| MONAMIC },	/* uppercase string */
	{ "xla", UFXLATE	| TRINAMIC },	/* XLATE character string translation */
} ;


/* When emacs' command interpetor needs to get a variable's name,
 * rather than it's value, it is passed back as a variable description
 * structure. The v_num field is a index into the appropriate variable table.
 */
typedef struct {
	int v_type ;	/* Type of variable. */
	int v_num ;		/* Ordinal pointer to variable in list. */
} variable_description ;

static void findvar( char *var, variable_description *vd, int size) ;
static int svar( variable_description *var, char *value) ;
static char *i_to_a( int i) ;

/*
 * putctext:
 *	replace the current line with the passed in text
 *
 * char *iline;			contents of new line
 */
static int putctext( char *iline)
{
	int status;

	/* delete the current line */
	curwp->w_doto = 0;	/* starting at the beginning of the line */
	if ((status = killtext(TRUE, 1)) != TRUE)
		return status;

	/* insert the new line */
	if ((status = linstr(iline)) != TRUE)
		return status;
	status = lnewline();
	backline(TRUE, 1);
	return status;
}


static char *result ;		/* string result */
static int ressize = 0 ;	/* mark result as uninitialized */

static int ernd( int i) ;
static int sindex( char *source, char *pattern) ;
static char *xlat( char *source, char *lookup, char *trans) ;

/* Initialize the user variable list. */
void varinit(void)
{
	int i;
	for (i = 0; i < MAXVARS; i++)
		uv[i].u_name[0] = 0;

	if( ressize == 0) {
		result = malloc( NSTRING) ;
		ressize = NSTRING ;
	}

	seed = time( NULL) ;
}


/* Evaluate a function.
 *
 * @fname: name of function to evaluate.
 */
static const char *gtfun( char *fname) {
	char *argv[ 3] ;
	const char *retstr ;		/* return value */
	int i ;

/* look the function up in the function table */
	fname[ 3] = 0 ;		/* only first 3 chars significant */
	mklower( fname) ;	/* and let it be upper or lower case */
	unsigned fnum = ARRAY_SIZE( funcs) ;	/* index to function to eval */
	int low = 0 ;				/* binary search low bound */
	int high = fnum - 1 ;		/* binary search high bound */
	do {
		int s, cur ;

		cur = (high + low) / 2 ;
		s = strcmp( fname, funcs[ cur].f_name) ;
		if( s == 0) {
			fnum = cur ;
			break ;
		} else if( s < 0)
			high = cur - 1 ;
		else
			low = cur + 1 ;
	} while( low <= high) ;

/* return errorm on a bad reference */
	if( fnum == ARRAY_SIZE( funcs))
		return errorm ;

/* fetch arguments */
	assert( clexec == TRUE) ; /* means macarg can be replaced by gettokval */
	int argc = ARGCOUNT( funcs[ fnum].f_type) ;
	for( int i = 0 ; i < argc ; i++) {
		argv[ i] = getnewtokval() ;
		if( argv[ i] == NULL) {
			while( i > 0)
				free( argv[ --i]) ;

			return errorm ;
		}
	}

/* and now evaluate it! */
	switch( FUNID( funcs[ fnum].f_type)) {
		int sz, sz1 ;
		unicode_t c ;

	case UFADD:
		retstr = i_to_a( atoi( argv[ 0]) + atoi( argv[ 1])) ;
		break ;
	case UFSUB:
		retstr = i_to_a( atoi( argv[ 0]) - atoi( argv[ 1])) ;
		break ;
	case UFTIMES:
		retstr = i_to_a( atoi( argv[ 0]) * atoi( argv[ 1])) ;
		break ;
	case UFDIV:
		sz = atoi( argv[ 1]) ;
		retstr = (sz == 0) ? errorm : i_to_a( atoi( argv[ 0]) / sz) ;
		break ;
	case UFMOD:
		sz = atoi( argv[ 1]) ;
		retstr = (sz == 0) ? errorm : i_to_a( atoi( argv[ 0]) % sz) ;
		break ;
	case UFNEG:
		retstr = i_to_a( -atoi( argv[ 0])) ;
		break ;
	case UFCAT:
		sz1 = strlen( argv[ 0]) ;
		sz = sz1 + strlen( argv[ 1]) + 1 ;
		if( sz > ressize) {
			free( result) ;
			result = malloc( sz) ;
			ressize = sz ;
		}

		strcpy( result, argv[ 0]) ;
		strcpy( &result[ sz1], argv[ 1]) ;
		retstr = result ;
		break ;
	case UFLEFT:
		sz1 = strlen( argv[ 0]) ;
		sz = 0 ;
		for( int i = atoi( argv[ 1]) ; i > 0 ; i -= 1) {
			unicode_t c ;

			sz += utf8_to_unicode( argv[ 0], sz, sz1, &c) ;
			if( sz == sz1)
				break ;
		}

		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}

		mystrscpy( result, argv[ 0], sz + 1) ;
		retstr = result ;
		break ;
	case UFRIGHT:
		sz = strlen( argv[ 0]) ;
		for( sz1 = atoi( argv[ 1]) ; sz1 > 0 && sz > 0 ; sz1--)
			if( --sz > 0)
				sz -= utf8_revdelta( (unsigned char *) &( argv[ 0])[ sz], sz) ;

		retstr = &( argv[ 0])[ sz] ;
		sz = strlen( retstr) ;
		if( sz >= ressize) {
			free( result) ;
			ressize = sz + 1 ;
			result = malloc( ressize) ;
		}

		retstr = strcpy( result, retstr) ;
		break ;
	case UFMID:
		sz1 = strlen( argv[ 0]) ;
		int start = 0 ;
		for( i = atoi( argv[ 1]) - 1 ; i > 0 ; i -= 1) {
			start +=  utf8_to_unicode( argv[ 0], start, sz1, &c) ;
			if( start == sz1)
				break ;
		}

		sz = start ;
		if( sz < sz1)
		for( i = atoi( argv[ 2]) ; i > 0 ; i -= 1) {
			sz += utf8_to_unicode( argv[ 0], sz, sz1, &c) ;
			if( sz == sz1)
				break ;
		}

		sz -= start ;
		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}

		mystrscpy( result, &(argv[ 0][ start]), sz + 1) ;
		retstr = result ;
		break ;
	case UFNOT:
		retstr = ltos( stol( argv[ 0]) == FALSE) ;
		break ;
	case UFEQUAL:
		retstr = ltos( atoi( argv[ 0]) == atoi( argv[ 1])) ;
		break ;
	case UFLESS:
		retstr = ltos( atoi( argv[ 0]) < atoi( argv[ 1])) ;
		break ;
	case UFGREATER:
		retstr = ltos( atoi( argv[ 0]) > atoi( argv[ 1])) ;
		break ;
	case UFSEQUAL:
		retstr = ltos( strcmp( argv[ 0], argv[ 1]) == 0) ;
		break ;
	case UFSLESS:
		retstr = ltos( strcmp( argv[ 0], argv[ 1]) < 0) ;
		break ;
	case UFSGREAT:
		retstr = ltos( strcmp( argv[ 0], argv[ 1]) > 0) ;
		break ;
	case UFIND:
		retstr = getval( argv[ 0]) ;
		sz = strlen( retstr) + 1 ;
		if( sz > ressize) {
			free( result) ;
			result = malloc( sz) ;
			ressize = sz ;
		}

		retstr = strcpy( result, retstr) ;
		break ;
	case UFAND:
		retstr = ltos( stol( argv[ 0]) && stol( argv[ 1])) ;
		break ;
	case UFOR:
		retstr = ltos( stol( argv[ 0]) || stol( argv[ 1])) ;
		break ;
	case UFLENGTH:
		retstr = i_to_a( strlen( argv[ 0])) ;
		break ;
	case UFUPPER:
		sz = strlen( argv[ 0]) ;
		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}

		retstr = mkupper( result, argv[ 0]) ;
		break ;
	case UFLOWER:
		sz = strlen( argv[ 0]) ;
		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}

		strcpy( result, argv[ 0]) ;	/* result is at least as long as argv[ 0] */
		retstr = mklower( result) ;
		break ;
	case UFTRUTH:
		retstr = ltos( atoi( argv[ 0]) == 42) ;
		break ;
	case UFASCII:
		utf8_to_unicode( argv[ 0], 0, 4, &c) ;
		retstr = i_to_a( c) ;
		break ;
	case UFCHR:
		c = atoi( argv[ 0]) ;
		if( c > 0x10FFFF)
			retstr = errorm ;
		else {
			sz = unicode_to_utf8( c, result) ;
			result[ sz] = 0 ;
			retstr = result ;
		}

		break ;
	case UFGTKEY:
		result[0] = tgetc();
		result[1] = 0;
		retstr = result ;
		break ;
	case UFRND:
		retstr = i_to_a( ernd( atoi( argv[ 0]))) ;
		break ;
	case UFABS:
		retstr = i_to_a( abs( atoi( argv[ 0]))) ;
		break ;
	case UFSINDEX:
		retstr = i_to_a( sindex( argv[ 0], argv[ 1])) ;
		break ;
	case UFENV:
#if	ENVFUNC
		retstr = getenv( argv[ 0]) ;
		if( retstr == NULL)
#endif
			retstr = "" ;

		break ;
	case UFBIND:
		retstr = transbind( argv[ 0]) ;
		break ;
	case UFEXIST:
		retstr = ltos( fexist( argv[ 0])) ;
		break ;
	case UFFIND:
		retstr = flook( argv[ 0], TRUE) ;
		if( retstr == NULL)
			retstr = "" ;
		break ;
	case UFBAND:
		retstr = i_to_a( atoi( argv[ 0]) & atoi( argv[ 1])) ;
		break ;
	case UFBOR:
		retstr = i_to_a( atoi( argv[ 0]) | atoi( argv[ 1])) ;
		break ;
	case UFBXOR:
		retstr = i_to_a( atoi( argv[ 0]) ^ atoi( argv[ 1])) ;
		break ;
	case UFBNOT:
		retstr = i_to_a( ~atoi( argv[ 0])) ;
		break ;
	case UFXLATE:
		retstr = xlat( argv[ 0], argv[ 1], argv[ 2]) ;
		break ;
	default:
		assert( FALSE) ;	/* never should get here */
		retstr = errorm ;
	}

	while( argc > 0)
		free( argv[ --argc]) ;

	return retstr ;
}

/*
 * look up a user var's value
 *
 * char *vname;			name of user variable to fetch
 */
static char *gtusr( char *vname) {
	int vnum;	/* ordinal number of user var */

	/* scan the list looking for the user var name */
	for (vnum = 0; vnum < MAXVARS; vnum++) {
		if (uv[vnum].u_name[0] == 0)
			break ;

		if( strncmp( vname, uv[ vnum].u_name, NVSIZE) == 0)
			return uv[vnum].u_value;
	}

	/* return errorm if we run off the end */
	return errorm;
}


/* gtenv()
 *
 * char *vname;			name of environment variable to retrieve
 */
static char *gtenv( char *vname) {
	unsigned vnum ;	/* ordinal number of var referenced */

	/* scan the list, looking for the referenced name */
	for( vnum = 0 ; vnum < ARRAY_SIZE( envars) ; vnum++)
		if( strcmp( vname, envars[ vnum]) == 0)
			break ;

	/* return errorm on a bad reference */
	if( vnum == ARRAY_SIZE( envars)) {
#if	ENVFUNC
		char *ename = getenv(vname);

		if( ename != NULL)
			return ename ;
#endif
		return errorm ;
	}

	/* otherwise, fetch the appropriate value */
	switch (vnum) {
	case EVFILLCOL:
		return i_to_a(fillcol);
	case EVPAGELEN:
		return i_to_a(term.t_nrow + 1);
	case EVCURCOL:
		return i_to_a(getccol(FALSE));
	case EVCURLINE:
		return i_to_a(getcline());
	case EVRAM:
		return i_to_a((int) (envram / 1024l));
	case EVFLICKER:
		return ltos(flickcode);
	case EVCURWIDTH:
		return i_to_a(term.t_ncol);
	case EVCBUFNAME:
		return curbp->b_bname;
	case EVCFNAME:
		return curbp->b_fname;
	case EVSRES:
		return sres;
	case EVDEBUG:
		return ltos(macbug);
	case EVSTATUS:
		return ltos(cmdstatus);
	case EVPALETTE: {
		static char palstr[ 49] = "" ; /* palette string */
		
		return palstr;
	    }

	case EVASAVE:
		return i_to_a(gasave);
	case EVACOUNT:
		return i_to_a(gacount);
	case EVLASTKEY:
		return i_to_a(lastkey);
	case EVCURCHAR: {
			unicode_t c ;

			lgetchar( &c) ;
			return i_to_a( c) ;
		}
	
	case EVDISCMD:
		return ltos(discmd);
	case EVVERSION:
		return VERSION;
	case EVPROGNAME:
		return PROGRAM_NAME_UTF8 ;
	case EVSEED:
		return i_to_a(seed);
	case EVDISINP:
		return ltos(disinp);
	case EVWLINE:
		return i_to_a(curwp->w_ntrows);
	case EVCWLINE:
		return i_to_a(getwpos());
	case EVTARGET:
		saveflag = lastflag;
		return i_to_a(curgoal);
	case EVSEARCH:
		return pat;
	case EVREPLACE:
		return rpat;
	case EVMATCH:
		return (patmatch == NULL) ? "" : patmatch;
	case EVKILL:
		return getkill();
	case EVCMODE:
		return i_to_a(curbp->b_mode);
	case EVGMODE:
		return i_to_a(gmode);
	case EVTPAUSE:
		return i_to_a(term.t_pause);
	case EVPENDING:
#if	TYPEAH
		return ltos(typahead());
#else
		return falsem;
#endif
	case EVLWIDTH:
		return i_to_a(llength(curwp->w_dotp));
	case EVLINE:
		return getctext();
	case EVGFLAGS:
		return i_to_a(gflags);
	case EVRVAL:
		return i_to_a(rval);
	case EVTAB:
		return i_to_a( tabwidth) ;
	case EVHARDTAB:
		return ltos( hardtab) ;
	case EVVIEWTAB:
		return ltos( viewtab) ;
	case EVOVERLAP:
		return i_to_a(overlap);
	case EVSCROLLCOUNT:
		return i_to_a(scrollcount);
#if SCROLLCODE
	case EVSCROLL:
		return ltos(term.t_scroll != NULL);
#else
	case EVSCROLL:
		return ltos(0);
#endif
	}

	assert( FALSE) ;	/* again, we should never get here */
	return errorm ;
}


/* set a variable
 *
 * int f;		default flag
 * int n;		numeric arg (can overide prompted value)
 */
BINDABLE( setvar) {
	int status;	/* status return */
	variable_description vd ;		/* variable num/type */
	char var[NVSIZE + 2];	/* name of variable to fetch %1234567890\0 */
	char *value ;			/* value to set variable to */

	/* first get the variable to set.. */
	if (clexec == FALSE) {
		status = getstring( "Variable to set: ", var, sizeof var, nlc) ;
		if (status != TRUE)
			return status;
	} else {		/* macro line argument */
		/* grab token and skip it */
		gettoken( var, sizeof var) ;
	}

	/* check the legality and find the var */
	findvar( var, &vd, sizeof var) ;

	/* if its not legal....bitch */
	if (vd.v_type == -1) {
		mlwrite("%%No such variable as '%s'", var);
		return FALSE;
	}

	/* get the value for that variable */
	if( f == TRUE) {
		value = malloc( NSTRING) ;
		if( value == NULL)
			return FALSE ;

		/* a bit overcautious here */
		mystrscpy( value, i_to_a( n), NSTRING) ;
	} else {
		status = newmlarg( &value, "Value: ", 0) ;
		if (status != TRUE)
			return status;
	}

	/* and set the appropriate value */
	status = svar(&vd, value);

#if	DEBUGM
	/* if $debug == TRUE, every assignment will echo a statment to
	   that effect here. */

	if( macbug)
		if( abortc == mdbugout( "(((%s:%s:%s)))", ltos( status), var, value))
			status = FALSE ;
#endif

	/* and return it */
	free( value) ;
	return status;
}

static void mlforce( char *s) ;

#if DEBUGM
int mdbugout( char *fmt, ...) {
	int	c ;	/* input from kbd, output to terminal */
	int savediscmd ;
	va_list	ap ;

	/* assignment status ; variable name ; value we tried to assign  */
	/* write out the debug line */
	savediscmd = discmd ;
	discmd = TRUE ;
	va_start( ap, fmt) ;
	vmlwrite( fmt, ap) ;
	va_end( ap) ;
	discmd = savediscmd ;
	update( TRUE) ;

	/* and get the keystroke to hold the output */
	c = get1key() ;
	if( c == abortc)
		mlforce( "(Macro aborted)") ;

	return c ;
}
#endif

/*
 * Find a variables type and name.
 *
 * @var: name of variable to get.
 * @vd: structure to hold type and pointer.
 * @size: size of variable array.
 */
static void findvar(char *var, variable_description *vd, int size)
{
	unsigned vnum = 0 ;	/* subscript in variable arrays */
	int vtype;	/* type to return */

fvar:
	vtype = -1;
	switch (var[0]) {

	case '$':		/* check for legal enviromnent var */
		for (vnum = 0; vnum < ARRAY_SIZE(envars); vnum++)
			if (strcmp(&var[1], envars[vnum]) == 0) {
				vtype = TKENV;
				break;
			}
		break;

	case '%':		/* check for existing legal user variable */
		for (vnum = 0; vnum < MAXVARS; vnum++)
			if (strcmp(&var[1], uv[vnum].u_name) == 0) {
				vtype = TKVAR;
				break;
			}
		if (vnum < MAXVARS)
			break;

		/* create a new one??? */
		for (vnum = 0; vnum < MAXVARS; vnum++)
			if (uv[vnum].u_name[0] == 0) {
				vtype = TKVAR;
				mystrscpy( uv[ vnum].u_name, &var[ 1], NVSIZE + 1) ;
				break;
			}
		break;

	case '&':		/* indirect operator? */
		var[4] = 0;
		if (strcmp(&var[1], "ind") == 0) {
			/* grab token, and eval it */
			if( TRUE == gettokval( var, size))
				goto fvar ;
		}
	}

	/* return the results */
	vd->v_num = vnum;
	vd->v_type = vtype;
	return;
}

/*
 * Set a variable.
 *
 * @var: variable to set.
 * @value: value to set to.
 */
static int svar( variable_description *var, char *value)
{
	int vnum;	/* ordinal number of var refrenced */
	int vtype;	/* type of variable to set */
	int status;	/* status return */
	int c;		/* translated character */
	char *sp;	/* scratch string pointer */

	/* simplify the vd structure (we are gonna look at it a lot) */
	vnum = var->v_num;
	vtype = var->v_type;

	/* and set the appropriate value */
	status = TRUE;
	switch (vtype) {
	case TKVAR:		/* set a user variable */
		if (uv[vnum].u_value != NULL)
			free(uv[vnum].u_value);
		sp = malloc(strlen(value) + 1);
		if (sp == NULL)
			return FALSE;
		strcpy(sp, value);
		uv[vnum].u_value = sp;
		break;

	case TKENV:		/* set an environment variable */
		status = TRUE;	/* by default */
		switch (vnum) {
		case EVFILLCOL:
			fillcol = atoi(value);
			break;
		case EVPAGELEN:
			status = newsize(TRUE, atoi(value));
			break;
		case EVCURCOL:
			status = setccol(atoi(value));
			break;
		case EVCURLINE:
			status = gotoline(TRUE, atoi(value));
			break;
		case EVRAM:
			break;
		case EVFLICKER:
			flickcode = stol(value);
			break;
		case EVCURWIDTH:
			status = newwidth(TRUE, atoi(value));
			break;
		case EVCBUFNAME:
			strcpy(curbp->b_bname, value);
			curwp->w_flag |= WFMODE;
			break;
		case EVCFNAME:
			strcpy(curbp->b_fname, value);
			curwp->w_flag |= WFMODE;
			break;
		case EVSRES:
			status = TTrez(value);
			break;
		case EVDEBUG:
			macbug = stol(value);
			break;
		case EVSTATUS:
			cmdstatus = stol(value);
			break;
		case EVASAVE:
			gasave = atoi(value);
			break;
		case EVACOUNT:
			gacount = atoi(value);
			break;
		case EVLASTKEY:
			lastkey = atoi(value);
			break;
		case EVCURCHAR:
			ldelchar(1, FALSE);	/* delete 1 char */
			c = atoi(value);
			if (c == '\n')
				lnewline();
			else
				linsert(1, c);
			backchar(FALSE, 1);
			break;
		case EVDISCMD:
			discmd = stol(value);
			break;
		case EVVERSION:
			break;
		case EVPROGNAME:
			break;
		case EVSEED:
			seed = atoi(value);
			break;
		case EVDISINP:
			disinp = stol(value);
			break;
		case EVWLINE:
			status = resize(TRUE, atoi(value));
			break;
		case EVCWLINE:
			status = forwline(TRUE, atoi(value) - getwpos());
			break;
		case EVTARGET:
			curgoal = atoi(value);
			thisflag = saveflag;
			break;
		case EVSEARCH:
			strcpy(pat, value);
			rvstrcpy(tap, pat);
#if	MAGIC
			mcclear();
#endif
			break;
		case EVREPLACE:
			strcpy(rpat, value);
			break;
		case EVMATCH:
			break;
		case EVKILL:
			break;
		case EVCMODE:
			curbp->b_mode = atoi(value);
			curwp->w_flag |= WFMODE;
			break;
		case EVGMODE:
			gmode = atoi(value);
			break;
		case EVTPAUSE:
			term.t_pause = atoi(value);
			break;
		case EVPENDING:
			break;
		case EVLWIDTH:
			break;
		case EVLINE:
			return putctext( value) ;
		case EVGFLAGS:
			gflags = atoi(value);
			break;
		case EVRVAL:
			break;
		case EVTAB:
			c = atoi( value) ;
			if( c > 0) {
				tabwidth = c ;
				curwp->w_flag |= WFHARD;
			} else
				status = FALSE ;

			break;
		case EVHARDTAB:
			hardtab = stol( value) ;
			break ;
		case EVVIEWTAB:
			viewtab = stol( value) ;
			break ;
		case EVOVERLAP:
			overlap = atoi(value);
			break;
		case EVSCROLLCOUNT:
			scrollcount = atoi(value);
			break;
		case EVSCROLL:
#if SCROLLCODE
			if (!stol(value))
				term.t_scroll = NULL;
#endif
			break;
		}
		break;
	}
	return status;
}

/*
 * i_to_a:
 *	integer to ascii string.......... This is too
 *	inconsistant to use the system's
 *
 * int i;		integer to translate to a string
 */
static char *i_to_a( int i) {
	unsigned u ;
	int sign ;	/* sign of resulting number */
	/* returns result string: sign digits null */
	static char result[ 1 + (sizeof i * 5 + 1) / 2 + 1] ;
	char *sp = &result[ sizeof result - 1] ; /* points on result's last byte */

	*sp = 0 ;

	/* record the sign... */
	sign = i < 0 ;
	u = sign ? -i : i ;

	/* and build the string (backwards!) */
	do {
		*(--sp) = '0' + u % 10 ;	/* install the new digit */
		u = u / 10 ;
	} while( u) ;

	/* and fix the sign */
	if( sign)
		*(--sp) = '-';	/* and install the minus sign */

	return sp ;
}


/* find the type of a token based on first character
 *
 * char c ; 	first character of analyzed token
 */
static int gettyp( char c) {
 	switch( c) {
	case '*':
	case ':':
		return TKLBL ;
	case 0:	/* no blanks!!! */
		return TKNUL ;
	case '"':
		return TKSTR ;
	case '!':
		return TKDIR ;
	case '@':
		return TKARG ;
	case '=':
		return TKBUF ;
	case '$':
		return TKENV ;
	case '%':
		return TKVAR ;
	case '&':
		return TKFUN ;

	default:
		/* a numeric literal? */
		if( (c >= '0' && c <= '9') || c == '-')
			return TKLIT ;
		else
			return TKCMD ;
	}
}

int is_it_cmd( char *token) {
	return TKCMD == gettyp( *token) ;
}


/* find the value of a token
 *
 * char *token;		token to evaluate
 */
const char *getval( char *token) {
	static char buf[ NSTRING] ;	/* string buffer for some returns */

	switch( gettyp( *token)) {
	case TKARG:		/* interactive argument */
		strcpy( token, getval( &token[ 1])) ;
		int distmp = discmd ;	/* echo it always! */
		discmd = TRUE ;
		int status = getstring( token, buf, NSTRING, nlc) ;
		discmd = distmp ;
		if (status == ABORT)
			return errorm ;

		return buf ;

	case TKBUF:		/* buffer contents fetch */
	/* grab the right buffer */
		strcpy( token, getval( &token[ 1])) ;
		buffer_p bp = bfind( token, FALSE, 0) ;
		if (bp == NULL)
			return errorm ;

	/* if the buffer is displayed,
	   get the window vars instead of the buffer vars */
		if (bp->b_nwnd > 0) {
			curbp->b_dotp = curwp->w_dotp;
			curbp->b_doto = curwp->w_doto;
		}

	/* make sure we are not at the end */
		if (bp->b_linep == bp->b_dotp)
			return errorm;

	/* grab the line as an argument */
		unsigned blen = bp->b_dotp->l_used - bp->b_doto;
		if( blen >= sizeof buf)
			blen = sizeof buf - 1 ;

		mystrscpy( buf, bp->b_dotp->l_text + bp->b_doto, blen + 1) ;

	/* and step the buffer's line ptr ahead a line */
		bp->b_dotp = bp->b_dotp->l_fp;
		bp->b_doto = 0;

	/* if displayed buffer, reset window ptr vars */
		if (bp->b_nwnd > 0) {
			curwp->w_dotp = curbp->b_dotp;
			curwp->w_doto = 0;
			curwp->w_flag |= WFMOVE;
		}

	/* and return the spoils */
		return buf;

	case TKVAR:
		return gtusr(token + 1);
	case TKENV:
		return gtenv(token + 1);
	case TKFUN:
		return gtfun(token + 1);
	case TKLIT:
		return token;
	case TKSTR:
		return token + 1;
	case TKCMD:
		return token;
	case TKDIR:
	case TKLBL:
	case TKNUL:
		return "" ;
	}

	return errorm ;
}

/*
 * convert a string to a numeric logical
 *
 * char *val;		value to check for stol
 */
int stol(char *val)
{
	/* check for logical values */
	if (val[0] == 'F')
		return FALSE;
	if (val[0] == 'T')
		return TRUE;

	/* check for numeric truth (!= 0) */
	return (atoi(val) != 0);
}

/*
 * numeric logical to string logical
 *
 * int val;		value to translate
 */
static char *ltos( int val) {
	static char *boolm[] = { "TRUE", "FALSE" } ;
	
	return boolm[ !val] ;
}

/*
 * make a string upper case
 *
 * char *src ;		string to upper case
 * char *dst ;		where to store
 * dst must be at least as long as src.
 */
static char *mkupper( char *dst, char *src) {
	char c, *sp ;

	sp = dst ;
	while( (c = *src++)) {
		if( 'a' <= c && c <= 'z')
			c += 'A' - 'a' ;
			
		*sp++ = c ;
	}
	
	*sp = 0 ;
	return dst ;
}

/*
 * make a string lower case
 *
 * char *str;		string to lower case
 */
char *mklower(char *str)
{
	char *sp;

	sp = str;
	while (*sp) {
		if ('A' <= *sp && *sp <= 'Z')
			*sp += 'a' - 'A';
		++sp;
	}
	return str;
}

/*
 * returns a random integer
 *	ernd( 0)			[ 0 .. 2147483647]
 *	ernd( -2147483648)	[ 0 .. 2147483647]
 *	ernd( 1) 			[ 1]
 *  ernd( i)			[ 1 .. abs( i)]
 */
static int ernd( int i) {
	int s ;

	seed = seed * 1721 + 10007 ;
	s = ((seed >> 16) & 0x0000FFFF) | (seed << 16) ;
	s &= ~(1 << 31) ; /* avoid abs() */
	i = i < 0 ? -i : i ;	/* abs( i) */
	return (i <= 0) ? s : s % i + 1 ;
}


/* find pattern within source
 *
 * char *source;	source string to search
 * char *pattern;	string to look for
 */
static int sindex( char *source, char *pattern) {
/* scanning through the source string */
	char *sp = source ;		/* ptr to current position to scan */
	int idx = 1 ;
	int pos = 0 ;
	int len = strlen( source) ;

	while( *sp) {
		char c ;
		unicode_t uc ;
		
		/* scan through the pattern */
		char *cp = pattern ;		/* ptr to place to check for equality */
		char *csp = sp ;			/* ptr to source string during comparison */

		while( (c = *cp++) && eq( c, *csp))
			csp++ ;

		/* was it a match? */
		if( c == 0)
			return idx ;

		idx += 1 ;
		pos += utf8_to_unicode( source, pos, len, &uc) ;
		sp = &source[ pos] ;
	}

	/* no match at all.. */
	return 0 ;
}

/*
 * Filter a string through a translation table
 *
 * char *source;	string to filter
 * char *lookup;	characters to translate
 * char *trans;		resulting translated characters
 */
static char *xlat( char *source, char *lookup, char *trans) {
	char *sp;	/* pointer into source table */
	char *lp;	/* pointer into lookup table */
	char *rp;	/* pointer into result */
	static char result[NSTRING];	/* temporary result */

	/* scan source string */
	sp = source;
	rp = result;
	while (*sp) {
		/* scan lookup table for a match */
		lp = lookup;
		while (*lp) {
			if (*sp == *lp) {
				*rp++ = trans[lp - lookup];
				goto xnext;
			}
			++lp;
		}

		/* no match, copy in the source char untranslated */
		*rp++ = *sp;

	      xnext:++sp;
	}

	/* terminate and return the result */
	*rp = 0;
	return result;
}

/*
 * Force a string out to the message line regardless of the
 * current $discmd setting. This is needed when $debug is TRUE
 * and for the write-message and clear-message-line commands
 *
 * char *s;		string to force out
 */
static void mlforce( char *s) {
	int oldcmd;	/* original command display flag */

	oldcmd = discmd;	/* save the discmd value */
	discmd = TRUE;		/* and turn display on */
	mlwrite( (*s) ? "%s" : "", s) ;	/* write the string out or erase line */
	discmd = oldcmd;	/* and restore the original setting */
}


/* This function simply clears the message line, mainly for macro usage
 *
 * int f, n;		arguments ignored
 */
TBINDABLE( clrmes) {
	mlforce( "") ;
	return TRUE ;
}


/* This function writes a string on the message line mainly for macro usage
 *
 * int f, n; arguments ignored
 */
BINDABLE( writemsg) {
	char *buf ; /* buffer to receive message into */

	int status = newmlarg( &buf, "write-message: ", 0) ;
	if( status == TRUE) {
	/* write the message out */
		mlforce( buf) ;
		free( buf) ;
	}

	return status ;
}


/* end of eval.c */
