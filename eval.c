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

#include "basic.h"
#include "bind.h"
#include "buffer.h"
#include "display.h"
#include "estruct.h"
#include "edef.h"
#include "exec.h"
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

#if	DEBUGM
/*	vars needed for macro debugging output	*/
char outline[ NSTRING] ;	/* global string to hold debug line text */
#endif

int gflags = GFREAD ;		/* global control flag */
int macbug = FALSE ;		/* macro debuging flag          */
int cmdstatus = TRUE ;		/* last command status          */

static int saveflag = 0 ;	/* Flags, saved with the $target var */


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


/* Initialize the user variable list. */
void varinit(void)
{
	int i;
	for (i = 0; i < MAXVARS; i++)
		uv[i].u_name[0] = 0;
}

/*
 * Evaluate a function.
 *
 * @fname: name of function to evaluate.
 */
char *gtfun(char *fname)
{
	int fnum;	/* index to function to eval */
	int status;	/* return status */
	char *tsp;	/* temporary string pointer */
	char arg1[NSTRING];	/* value of first argument */
	char arg2[NSTRING];	/* value of second argument */
	char arg3[NSTRING];	/* value of third argument */
	static char result[2 * NSTRING];	/* string result */

	/* look the function up in the function table */
	fname[3] = 0;		/* only first 3 chars significant */
	mklower(fname);		/* and let it be upper or lower case */
	for (fnum = 0; fnum < ARRAY_SIZE(funcs); fnum++)
		if (strcmp(fname, funcs[fnum].f_name) == 0)
			break;

	/* return errorm on a bad reference */
	if (fnum == ARRAY_SIZE(funcs))
		return errorm;

	/* if needed, retrieve the first argument */
	if (funcs[fnum].f_type >= MONAMIC) {
		if ((status = macarg(arg1)) != TRUE)
			return errorm;

		/* if needed, retrieve the second argument */
		if (funcs[fnum].f_type >= DYNAMIC) {
			if ((status = macarg(arg2)) != TRUE)
				return errorm;

			/* if needed, retrieve the third argument */
			if (funcs[fnum].f_type >= TRINAMIC)
				if ((status = macarg(arg3)) != TRUE)
					return errorm;
		}
	}


	/* and now evaluate it! */
	switch (fnum) {
	case UFADD:
		return itoa(atoi(arg1) + atoi(arg2));
	case UFSUB:
		return itoa(atoi(arg1) - atoi(arg2));
	case UFTIMES:
		return itoa(atoi(arg1) * atoi(arg2));
	case UFDIV:
		return itoa(atoi(arg1) / atoi(arg2));
	case UFMOD:
		return itoa(atoi(arg1) % atoi(arg2));
	case UFNEG:
		return itoa(-atoi(arg1));
	case UFCAT:
		strcpy(result, arg1);
		return strcat(result, arg2);
	case UFLEFT:
		return strncpy(result, arg1, atoi(arg2));
	case UFRIGHT:
		return (strcpy(result,
			       &arg1[(strlen(arg1) - atoi(arg2))]));
	case UFMID:
		return (strncpy(result, &arg1[atoi(arg2) - 1],
				atoi(arg3)));
	case UFNOT:
		return ltos(stol(arg1) == FALSE);
	case UFEQUAL:
		return ltos(atoi(arg1) == atoi(arg2));
	case UFLESS:
		return ltos(atoi(arg1) < atoi(arg2));
	case UFGREATER:
		return ltos(atoi(arg1) > atoi(arg2));
	case UFSEQUAL:
		return ltos(strcmp(arg1, arg2) == 0);
	case UFSLESS:
		return ltos(strcmp(arg1, arg2) < 0);
	case UFSGREAT:
		return ltos(strcmp(arg1, arg2) > 0);
	case UFIND:
		return strcpy(result, getval(arg1));
	case UFAND:
		return ltos(stol(arg1) && stol(arg2));
	case UFOR:
		return ltos(stol(arg1) || stol(arg2));
	case UFLENGTH:
		return itoa(strlen(arg1));
	case UFUPPER:
		return mkupper(arg1);
	case UFLOWER:
		return mklower(arg1);
	case UFTRUTH:
		return ltos(atoi(arg1) == 42);
	case UFASCII:
		return itoa((int) arg1[0]);
	case UFCHR:
		result[0] = atoi(arg1);
		result[1] = 0;
		return result;
	case UFGTKEY:
		result[0] = tgetc();
		result[1] = 0;
		return result;
	case UFRND:
		return itoa((ernd() % abs(atoi(arg1))) + 1);
	case UFABS:
		return itoa(abs(atoi(arg1)));
	case UFSINDEX:
		return itoa(sindex(arg1, arg2));
	case UFENV:
#if	ENVFUNC
		tsp = getenv(arg1);
		return tsp == NULL ? "" : tsp;
#else
		return "";
#endif
	case UFBIND:
		return transbind(arg1);
	case UFEXIST:
		return ltos(fexist(arg1));
	case UFFIND:
		tsp = flook(arg1, TRUE);
		return tsp == NULL ? "" : tsp;
	case UFBAND:
		return itoa(atoi(arg1) & atoi(arg2));
	case UFBOR:
		return itoa(atoi(arg1) | atoi(arg2));
	case UFBXOR:
		return itoa(atoi(arg1) ^ atoi(arg2));
	case UFBNOT:
		return itoa(~atoi(arg1));
	case UFXLATE:
		return xlat(arg1, arg2, arg3);
	}

	exit(-11);		/* never should get here */
}

/*
 * look up a user var's value
 *
 * char *vname;			name of user variable to fetch
 */
char *gtusr(char *vname)
{

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
char *gtenv(char *vname)
{
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
		return itoa(fillcol);
	case EVPAGELEN:
		return itoa(term.t_nrow + 1);
	case EVCURCOL:
		return itoa(getccol(FALSE));
	case EVCURLINE:
		return itoa(getcline());
	case EVRAM:
		return itoa((int) (envram / 1024l));
	case EVFLICKER:
		return ltos(flickcode);
	case EVCURWIDTH:
		return itoa(term.t_ncol);
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
		return itoa(gasave);
	case EVACOUNT:
		return itoa(gacount);
	case EVLASTKEY:
		return itoa(lastkey);
	case EVCURCHAR:
		return (curwp->w_dotp->l_used ==
			curwp->w_doto ? itoa('\n') :
			itoa(lgetc(curwp->w_dotp, curwp->w_doto)));
	case EVDISCMD:
		return ltos(discmd);
	case EVVERSION:
		return VERSION;
	case EVPROGNAME:
		return PROGRAM_NAME_LONG;
	case EVSEED:
		return itoa(seed);
	case EVDISINP:
		return ltos(disinp);
	case EVWLINE:
		return itoa(curwp->w_ntrows);
	case EVCWLINE:
		return itoa(getwpos());
	case EVTARGET:
		saveflag = lastflag;
		return itoa(curgoal);
	case EVSEARCH:
		return pat;
	case EVREPLACE:
		return rpat;
	case EVMATCH:
		return (patmatch == NULL) ? "" : patmatch;
	case EVKILL:
		return getkill();
	case EVCMODE:
		return itoa(curbp->b_mode);
	case EVGMODE:
		return itoa(gmode);
	case EVTPAUSE:
		return itoa(term.t_pause);
	case EVPENDING:
#if	TYPEAH
		return ltos(typahead());
#else
		return falsem;
#endif
	case EVLWIDTH:
		return itoa(llength(curwp->w_dotp));
	case EVLINE:
		return getctext();
	case EVGFLAGS:
		return itoa(gflags);
	case EVRVAL:
		return itoa(rval);
	case EVTAB:
		return itoa(tabmask + 1);
	case EVOVERLAP:
		return itoa(overlap);
	case EVSCROLLCOUNT:
		return itoa(scrollcount);
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
	char value[NSTRING];	/* value to set variable to */

	/* first get the variable to set.. */
	if (clexec == FALSE) {
		status = mlreply("Variable to set: ", &var[0], NVSIZE);
		if (status != TRUE)
			return status;
	} else {		/* macro line argument */
		/* grab token and skip it */
		execstr = token(execstr, var, NVSIZE + 1);
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
		strcpy(value, itoa(n));
	else {
		status = mlreply("Value: ", &value[0], NSTRING);
		if (status != TRUE)
			return status;
	}

	/* and set the appropriate value */
	status = svar(&vd, value);

#if	DEBUGM
	/* if $debug == TRUE, every assignment will echo a statment to
	   that effect here. */

	if (macbug) {
		char *sp ;	/* temp string pointer */

		strcpy(outline, "(((");

		/* assignment status */
		strcat(outline, ltos(status));
		strcat(outline, ":");

		/* variable name */
		strcat(outline, var);
		strcat(outline, ":");

		/* and lastly the value we tried to assign */
		strcat(outline, value);
		strcat(outline, ")))");

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
		if (get1key() == abortc) {
			mlforce("(Macro aborted)");
			status = FALSE;
		}
	}
#endif

	/* and return it */
	return status;
}

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
			execstr = token(execstr, var, size);
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
 * itoa:
 *	integer to ascii string.......... This is too
 *	inconsistant to use the system's
 *
 * int i;		integer to translate to a string
 */
char *itoa(int i)
{
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
int gettyp(char *token)
{
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
char *mkupper(char *str)
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
int ernd(void)
{
	seed = abs(seed * 1721 + 10007);
	return seed;
}

/*
 * find pattern within source
 *
 * char *source;	source string to search
 * char *pattern;	string to look for
 */
int sindex(char *source, char *pattern)
{
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
char *xlat(char *source, char *lookup, char *trans)
{
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
