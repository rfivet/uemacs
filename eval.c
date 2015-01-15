/* eval.c -- implements eval.h */
#include "eval.h"

/*	eval.c
 *
 *	Expression evaluation functions
 *
 *	written 1986 by Daniel Lawrence
 *	modified by Petri Kutvonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basic.h"
#include "bind.h"
#include "buffer.h"
#include "display.h"
#include "estruct.h"
#include "exec.h"
#include "execute.h"
#include "flook.h"
#include "input.h"
#include "line.h"
#include "random.h"
#include "search.h"
#include "terminal.h"
#include "termio.h"
#include "version.h"
#include "window.h"

#define	MAXVARS	255

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

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

static int gettyp( char *token) ;

/* Emacs global flag bit definitions (for gflags). */
/* if GFREAD is set, current buffer will be set on first file (read in) */
#define	GFREAD	1

static int gflags = GFREAD ;	/* global control flag		*/

int readfirst_f( void) {
	return GFREAD == (gflags & GFREAD) ;
}

int macbug = FALSE ;		/* macro debuging flag          */
int cmdstatus = TRUE ;		/* last command status          */
int flickcode = FALSE ;		/* do flicker supression?       */
int rval = 0 ;			/* return value of a subprocess */


static int saveflag = 0 ;	/* Flags, saved with the $target var */


long envram = 0l ;		/* # of bytes current in use by malloc */


/* Max #chars in a var name. */
#define	NVSIZE	10

/* Structure to hold user variables and their definitions. */
struct user_variable {
	char u_name[NVSIZE + 1]; /* name of user variable */
	char *u_value;		 /* value (string) */
};

static char errorm[] = "ERROR" ;	/* error literal                */

static int seed = 0 ;			/* random number seed           */

static char *ltos( int val) ;
static char *mkupper( char *str) ;

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
	"tab",			/* tab 4 or 8 */
	"overlap",
	"jump",
#if SCROLLCODE
	"scroll",		/* scroll enabled */
#endif
};

/* And its preprocesor definitions. */

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
#define EVOVERLAP	38
#define EVSCROLLCOUNT	39
#define EVSCROLL	40

enum function_type {
	NILNAMIC = 0,
	MONAMIC,
	DYNAMIC,
	TRINAMIC,
};

/* List of recognized user functions. */
static struct {
	const char f_name[ 4] ;
	const enum function_type f_type ;
} funcs[] = {
	{ "add", DYNAMIC },	/* add two numbers together */
	{ "sub", DYNAMIC },	/* subtraction */
	{ "tim", DYNAMIC },	/* multiplication */
	{ "div", DYNAMIC },	/* division */
	{ "mod", DYNAMIC },	/* mod */
	{ "neg", MONAMIC },	/* negate */
	{ "cat", DYNAMIC },	/* concatinate string */
	{ "lef", DYNAMIC },	/* left string(string, len) */
	{ "rig", DYNAMIC },	/* right string(string, pos) */
	{ "mid", TRINAMIC },	/* mid string(string, pos, len) */
	{ "not", MONAMIC },	/* logical not */
	{ "equ", DYNAMIC },	/* logical equality check */
	{ "les", DYNAMIC },	/* logical less than */
	{ "gre", DYNAMIC },	/* logical greater than */
	{ "seq", DYNAMIC },	/* string logical equality check */
	{ "sle", DYNAMIC },	/* string logical less than */
	{ "sgr", DYNAMIC },	/* string logical greater than */
	{ "ind", MONAMIC },	/* evaluate indirect value */
	{ "and", DYNAMIC },	/* logical and */
	{ "or", DYNAMIC },	/* logical or */
	{ "len", MONAMIC },	/* string length */
	{ "upp", MONAMIC },	/* uppercase string */
	{ "low", MONAMIC },	/* lower case string */
	{ "tru", MONAMIC },	/* Truth of the universe logical test */
	{ "asc", MONAMIC },	/* char to integer conversion */
	{ "chr", MONAMIC },	/* integer to char conversion */
	{ "gtk", NILNAMIC },	/* get 1 charater */
	{ "rnd", MONAMIC },	/* get a random number */
	{ "abs", MONAMIC },	/* absolute value of a number */
	{ "sin", DYNAMIC },	/* find the index of one string in another */
	{ "env", MONAMIC },	/* retrieve a system environment var */
	{ "bin", MONAMIC },	/* loopup what function name is bound to a key */
	{ "exi", MONAMIC },	/* check if a file exists */
	{ "fin", MONAMIC },	/* look for a file on the path... */
	{ "ban", DYNAMIC },	/* bitwise and   9-10-87  jwm */
	{ "bor", DYNAMIC },	/* bitwise or    9-10-87  jwm */
	{ "bxo", DYNAMIC },	/* bitwise xor   9-10-87  jwm */
	{ "bno", MONAMIC },	/* bitwise not */
	{ "xla", TRINAMIC },	/* XLATE character string translation */
};

/* And its preprocesor definitions. */

#define	UFADD		0
#define	UFSUB		1
#define	UFTIMES		2
#define	UFDIV		3
#define	UFMOD		4
#define	UFNEG		5
#define	UFCAT		6
#define	UFLEFT		7
#define	UFRIGHT		8
#define	UFMID		9
#define	UFNOT		10
#define	UFEQUAL		11
#define	UFLESS		12
#define	UFGREATER	13
#define	UFSEQUAL	14
#define	UFSLESS		15
#define	UFSGREAT	16
#define	UFIND		17
#define	UFAND		18
#define	UFOR		19
#define	UFLENGTH	20
#define	UFUPPER		21
#define	UFLOWER		22
#define	UFTRUTH		23
#define	UFASCII		24
#define	UFCHR		25
#define	UFGTKEY		26
#define	UFRND		27
#define	UFABS		28
#define	UFSINDEX	29
#define	UFENV		30
#define	UFBIND		31
#define	UFEXIST		32
#define	UFFIND		33
#define UFBAND		34
#define UFBOR		35
#define UFBXOR		36
#define	UFBNOT		37
#define	UFXLATE		38

/* User variables */
static struct user_variable uv[MAXVARS + 1];

/* When emacs' command interpetor needs to get a variable's name,
 * rather than it's value, it is passed back as a variable description
 * structure. The v_num field is a index into the appropriate variable table.
 */
struct variable_description {
	int v_type;  /* Type of variable. */
	int v_num;   /* Ordinal pointer to variable in list. */
};

static void findvar( char *var, struct variable_description *vd, int size) ;
static int svar( struct variable_description *var, char *value) ;

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

static int ernd( void) ;
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
}

/*
 * Evaluate a function.
 *
 * @fname: name of function to evaluate.
 */
static char *gtfun( char *fname) {
	int fnum ;					/* index to function to eval */
	char argx[ 512] ;			/* last argument, fixed sized allocation */
	char *arg1 ; 				/* value of first argument */
	char *arg2 ; 				/* value of second argument */
	char *retstr ;				/* return value */

	/* look the function up in the function table */
	fname[3] = 0;		/* only first 3 chars significant */
	mklower(fname);		/* and let it be upper or lower case */
	for (fnum = 0; fnum < ARRAY_SIZE(funcs); fnum++)
		if (strcmp(fname, funcs[fnum].f_name) == 0)
			break;

	/* return errorm on a bad reference */
	if (fnum == ARRAY_SIZE(funcs))
		return errorm;

	arg1 = arg2 = NULL ;

	/* if needed, retrieve the first argument */
	if (funcs[fnum].f_type >= MONAMIC) {
		if( macarg( argx, sizeof argx) != TRUE)
			return errorm;

		/* if needed, retrieve the second argument */
		if (funcs[fnum].f_type >= DYNAMIC) {
			arg1 = malloc( strlen( argx) + 1) ;
			strcpy( arg1, argx) ;
			if( macarg( argx, sizeof argx) != TRUE) {
				free( arg1) ;
				return errorm;
			}

			/* if needed, retrieve the third argument */
			if (funcs[fnum].f_type >= TRINAMIC) {
				arg2 = malloc( strlen( argx) + 1) ;
				strcpy( arg2, argx) ;
				if( macarg( argx, sizeof argx) != TRUE) {
					free( arg1) ;
					free( arg2) ;
					return errorm;
				}
			}
		}
	}

	/* and now evaluate it! */
	switch (fnum) {
		int sz ;

	case UFADD:
		retstr = i_to_a( atoi( arg1) + atoi( argx)) ;
		break ;
	case UFSUB:
		retstr = i_to_a( atoi( arg1) - atoi( argx)) ;
		break ;
	case UFTIMES:
		retstr = i_to_a( atoi( arg1) * atoi( argx)) ;
		break ;
	case UFDIV:
		retstr = i_to_a( atoi( arg1) / atoi( argx)) ;
		break ;
	case UFMOD:
		retstr = i_to_a( atoi( arg1) % atoi( argx)) ;
		break ;
	case UFNEG:
		retstr = i_to_a( -atoi( argx)) ;
		break ;
	case UFCAT: {
		int sz1 ;

		sz1 = strlen( arg1) ;
		sz = sz1 + strlen( argx) + 1 ;
		if( sz > ressize) {
			free( result) ;
			result = malloc( sz) ;
			ressize = sz ;
		}

		strcpy( result, arg1) ;
		strcpy( &result[ sz1], argx) ;
		retstr = result ;
	}
		break ;
	case UFLEFT:
		sz = atoi( argx) ;
		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}

		strncpy( result, arg1, sz) ;
		result[ sz] = 0 ;
		retstr = result ;
		break ;
	case UFRIGHT:
		sz = atoi( argx) ;
		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}
		
		retstr = strcpy( result, &arg1[ strlen( arg1) - sz]) ;
		break ;
	case UFMID:
		sz = atoi( argx) ;
		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}

		strncpy( result, &arg1[ atoi( arg2) - 1], sz) ;
		result[ sz] = 0 ;
		retstr = result ;
		break ;
	case UFNOT:
		retstr = ltos( stol( argx) == FALSE) ;
		break ;
	case UFEQUAL:
		retstr = ltos( atoi( arg1) == atoi( argx)) ;
		break ;
	case UFLESS:
		retstr = ltos( atoi( arg1) < atoi( argx)) ;
		break ;
	case UFGREATER:
		retstr = ltos( atoi( arg1) > atoi( argx)) ;
		break ;
	case UFSEQUAL:
		retstr = ltos( strcmp( arg1, argx) == 0) ;
		break ;
	case UFSLESS:
		retstr = ltos( strcmp( arg1, argx) < 0) ;
		break ;
	case UFSGREAT:
		retstr = ltos( strcmp( arg1, argx) > 0) ;
		break ;
	case UFIND:
		retstr = strcpy( result, getval( argx)) ;
		break ;
	case UFAND:
		retstr = ltos( stol( arg1) && stol( argx)) ;
		break ;
	case UFOR:
		retstr = ltos( stol( arg1) || stol( argx)) ;
		break ;
	case UFLENGTH:
		retstr = i_to_a( strlen( argx)) ;
		break ;
	case UFUPPER:
		sz = strlen( argx) ;
		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}

		strcpy( result, argx) ;	/* result is at least as long as argx */
		retstr = mkupper( result) ;
		break ;
	case UFLOWER:
		sz = strlen( argx) ;
		if( sz >= ressize) {
			free( result) ;
			result = malloc( sz + 1) ;
			ressize = sz + 1 ;
		}

		strcpy( result, argx) ;	/* result is at least as long as argx */
		retstr = mklower( result) ;
		break ;
	case UFTRUTH:
		retstr = ltos( atoi( argx) == 42) ;
		break ;
	case UFASCII:
		retstr = i_to_a( (int) argx[ 0]) ;
		break ;
	case UFCHR:
		result[0] = atoi(argx);
		result[1] = 0;
		retstr = result ;
		break ;
	case UFGTKEY:
		result[0] = tgetc();
		result[1] = 0;
		retstr = result ;
		break ;
	case UFRND:
		retstr = i_to_a( (ernd() % abs( atoi( argx))) + 1) ;
		break ;
	case UFABS:
		retstr = i_to_a( abs( atoi( argx))) ;
		break ;
	case UFSINDEX:
		retstr = i_to_a( sindex( arg1, argx)) ;
		break ;
	case UFENV:
#if	ENVFUNC
		retstr = getenv( argx) ;
		if( retstr == NULL)
			retstr = "" ;
#else
		retstr = "" ;
#endif
		break ;
	case UFBIND:
		retstr = transbind( argx) ;
		break ;
	case UFEXIST:
		retstr = ltos( fexist( argx)) ;
		break ;
	case UFFIND:
		retstr = flook( argx, TRUE) ;
		if( retstr == NULL)
			retstr = "" ;
		break ;
	case UFBAND:
		retstr = i_to_a( atoi( arg1) & atoi( argx)) ;
		break ;
	case UFBOR:
		retstr = i_to_a( atoi( arg1) | atoi( argx)) ;
		break ;
	case UFBXOR:
		retstr = i_to_a( atoi( arg1) ^ atoi( argx)) ;
		break ;
	case UFBNOT:
		retstr = i_to_a( ~atoi( argx)) ;
		break ;
	case UFXLATE:
		retstr = xlat( arg1, arg2, argx) ;
		break ;
	default:
		exit(-11);		/* never should get here */
	}

	if( arg2)
		free( arg2) ;

	if( arg1)
		free( arg1) ;

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
			return errorm;
		if (strcmp(vname, uv[vnum].u_name) == 0)
			return uv[vnum].u_value;
	}

	/* return errorm if we run off the end */
	return errorm;
}

/*
 * gtenv()
 *
 * char *vname;			name of environment variable to retrieve
 */
static char *gtenv( char *vname) {
	int vnum;	/* ordinal number of var refrenced */

	/* scan the list, looking for the referenced name */
	for (vnum = 0; vnum < ARRAY_SIZE(envars); vnum++)
		if (strcmp(vname, envars[vnum]) == 0)
			break;

	/* return errorm on a bad reference */
	if (vnum == ARRAY_SIZE(envars))
#if	ENVFUNC
	{
		char *ename = getenv(vname);

		if (ename != NULL)
			return ename;
		else
			return errorm;
	}
#else
		return errorm;
#endif

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
	case EVCURCHAR:
		return (curwp->w_dotp->l_used ==
			curwp->w_doto ? i_to_a('\n') :
			i_to_a(lgetc(curwp->w_dotp, curwp->w_doto)));
	case EVDISCMD:
		return ltos(discmd);
	case EVVERSION:
		return VERSION;
	case EVPROGNAME:
		return PROGRAM_NAME_LONG;
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
		return i_to_a(tabmask + 1);
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
	exit(-12);		/* again, we should never get here */
}

/*
 * set a variable
 *
 * int f;		default flag
 * int n;		numeric arg (can overide prompted value)
 */
int setvar(int f, int n)
{
	int status;	/* status return */
	struct variable_description vd;		/* variable num/type */
	char var[NVSIZE + 1];	/* name of variable to fetch */
	char value[ 2 * NSTRING] ;	/* value to set variable to */

	/* first get the variable to set.. */
	if (clexec == FALSE) {
		status = mlreply("Variable to set: ", &var[0], NVSIZE);
		if (status != TRUE)
			return status;
	} else {		/* macro line argument */
		/* grab token and skip it */
		gettoken( var, sizeof var) ;
	}

	/* check the legality and find the var */
	findvar(var, &vd, NVSIZE + 1);

	/* if its not legal....bitch */
	if (vd.v_type == -1) {
		mlwrite("%%No such variable as '%s'", var);
		return FALSE;
	}

	/* get the value for that variable */
	if (f == TRUE)
		strcpy(value, i_to_a(n));
	else {
		status = mlreply( "Value: ", value, sizeof value);
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
	return status;
}

#if DEBUGM
int mdbugout( char *fmt, char *s1, char *s2, char *s3) {
	char outline[ NSTRING] ;	/* global string to hold debug line text */
	int	c ;		/* input from kbd */
	char *sp ;	/* temp string pointer */

	/* assignment status ; variable name ; value we tried to assign  */
	sprintf( outline, fmt, s1, s2, s3) ;

	/* expand '%' to "%%" so mlwrite wont bitch */
	sp = outline;
	while (*sp)
		if (*sp++ == '%') {
			char *ep ;	/* ptr to end of outline */

			/* advance to the end */
			ep = --sp;
			while (*ep++);
			/* null terminate the string one out */
			*(ep + 1) = 0;
			/* copy backwards */
			while (ep-- > sp)
				*(ep + 1) = *ep;

			/* and advance sp past the new % */
			sp += 2;
		}

	/* write out the debug line */
	mlforce(outline);
	update(TRUE);

	/* and get the keystroke to hold the output */
	c = get1key() ;
	if( c == abortc)
		mlforce("(Macro aborted)");

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
static void findvar(char *var, struct variable_description *vd, int size)
{
	int vnum;	/* subscript in variable arrays */
	int vtype;	/* type to return */

	vnum = -1;
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
				strcpy(uv[vnum].u_name, &var[1]);
				break;
			}
		break;

	case '&':		/* indirect operator? */
		var[4] = 0;
		if (strcmp(&var[1], "ind") == 0) {
			/* grab token, and eval it */
			gettoken( var, size) ;
			strcpy(var, getval(var));
			goto fvar;
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
static int svar(struct variable_description *var, char *value)
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
			putctext(value);
		case EVGFLAGS:
			gflags = atoi(value);
			break;
		case EVRVAL:
			break;
		case EVTAB:
			tabmask = atoi(value) - 1;
			if (tabmask != 0x07 && tabmask != 0x03)
				tabmask = 0x07;
			curwp->w_flag |= WFHARD;
			break;
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
char *i_to_a(int i)
{
	#define	INTWIDTH	sizeof( int) * 3

	int digit;	/* current digit being used */
	char *sp;	/* pointer into result */
	int sign;	/* sign of resulting number */
	static char result[INTWIDTH + 1];	/* resulting string */

	/* record the sign... */
	sign = 1;
	if (i < 0) {
		sign = -1;
		i = -i;
	}

	/* and build the string (backwards!) */
	sp = result + INTWIDTH;
	*sp = 0;
	do {
		digit = i % 10;
		*(--sp) = '0' + digit;	/* and install the new digit */
		i = i / 10;
	} while (i);

	/* and fix the sign */
	if (sign == -1) {
		*(--sp) = '-';	/* and install the minus sign */
	}

	return sp;
}

/*
 * find the type of a passed token
 *
 * char *token;		token to analyze
 */
static int gettyp( char *token) {
	char c;	/* first char in token */

	/* grab the first char (this is all we need) */
	c = *token;

	/* no blanks!!! */
	if (c == 0)
		return TKNUL;

	/* a numeric literal? */
	if (c >= '0' && c <= '9')
		return TKLIT;

	switch (c) {
	case '"':
		return TKSTR;

	case '!':
		return TKDIR;
	case '@':
		return TKARG;
	case '#':
		return TKBUF;
	case '$':
		return TKENV;
	case '%':
		return TKVAR;
	case '&':
		return TKFUN;
	case '*':
		return TKLBL;

	default:
		return TKCMD;
	}
}

int is_it_cmd( char *token) {
	return TKCMD == gettyp( token) ;
}

/*
 * find the value of a token
 *
 * char *token;		token to evaluate
 */
char *getval(char *token)
{
	int status;	/* error return */
	struct buffer *bp;	/* temp buffer pointer */
	int blen;	/* length of buffer argument */
	int distmp;	/* temporary discmd flag */
	static char buf[NSTRING];	/* string buffer for some returns */

	switch (gettyp(token)) {
	case TKNUL:
		return "";

	case TKARG:		/* interactive argument */
		strcpy(token, getval(&token[1]));
		distmp = discmd;	/* echo it always! */
		discmd = TRUE;
		status = getstring(token, buf, NSTRING, ctoec('\n'));
		discmd = distmp;
		if (status == ABORT)
			return errorm;
		return buf;

	case TKBUF:		/* buffer contents fetch */

		/* grab the right buffer */
		strcpy(token, getval(&token[1]));
		bp = bfind(token, FALSE, 0);
		if (bp == NULL)
			return errorm;

		/* if the buffer is displayed, get the window
		   vars instead of the buffer vars */
		if (bp->b_nwnd > 0) {
			curbp->b_dotp = curwp->w_dotp;
			curbp->b_doto = curwp->w_doto;
		}

		/* make sure we are not at the end */
		if (bp->b_linep == bp->b_dotp)
			return errorm;

		/* grab the line as an argument */
		blen = bp->b_dotp->l_used - bp->b_doto;
		if (blen > NSTRING)
			blen = NSTRING;
		strncpy(buf, bp->b_dotp->l_text + bp->b_doto, blen);
		buf[blen] = 0;

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
	case TKDIR:
		return errorm;
	case TKLBL:
		return errorm;
	case TKLIT:
		return token;
	case TKSTR:
		return token + 1;
	case TKCMD:
		return token;
	}
	return errorm;
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
static char *ltos( int val)
{
	static char truem[] = "TRUE" ;		/* true literal  */
	static char falsem[] = "FALSE" ;	/* false literal */

	if (val)
		return truem;
	else
		return falsem;
}

/*
 * make a string upper case
 *
 * char *str;		string to upper case
 */
static char *mkupper( char *str)
{
	char *sp;

	sp = str;
	while (*sp) {
		if ('a' <= *sp && *sp <= 'z')
			*sp += 'A' - 'a';
		++sp;
	}
	return str;
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
 * take the absolute value of an integer
 */
int abs(int x)
{
	return x < 0 ? -x : x;
}

/*
 * returns a random integer
 */
static int ernd( void) {
	seed = abs(seed * 1721 + 10007);
	return seed;
}

/*
 * find pattern within source
 *
 * char *source;	source string to search
 * char *pattern;	string to look for
 */
static int sindex( char *source, char *pattern) {
	char *sp;		/* ptr to current position to scan */
	char *csp;		/* ptr to source string during comparison */
	char *cp;		/* ptr to place to check for equality */

	/* scanning through the source string */
	sp = source;
	while (*sp) {
		/* scan through the pattern */
		cp = pattern;
		csp = sp;
		while (*cp) {
			if (!eq(*cp, *csp))
				break;
			++cp;
			++csp;
		}

		/* was it a match? */
		if (*cp == 0)
			return (int) (sp - source) + 1;
		++sp;
	}

	/* no match at all.. */
	return 0;
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
