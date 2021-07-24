/* names.h -- mapping of functions to names and keys */

#ifndef _NAMES_H_
#define _NAMES_H_

/* Bindable uEMACS function pointer type and definition template */
#define BINDABLE( fname)	int fname( int f, int n)

typedef BINDABLE( (*fnp_t)) ;


/* Structure for the name binding table. */
typedef struct {
	const char	*n_name ;	/* name starting with one tag character */
	fnp_t		n_func ;	/* function the name is bound to */
	unsigned	n_keycode ;	/* default key assignment, 0 when none */
} name_bind ;

#define bind_name( p) (&( p)->n_name[ 1])
#define bind_tag( p)  ( p)->n_name[ 0]

/* Structure for the key bindings table. */
typedef struct {
	unsigned		k_code ;		/* Key code */
	const name_bind	*k_nbp ;		/* entry in name to function map table */
} key_tab ;


extern const name_bind names[] ;	/* name to function mapping table */

/* keycode to function mapping table */
#define	NBINDS	256					/* max # of bound keys          */
extern key_tab keytab[ NBINDS] ;	/* key bind to functions table  */


void init_bindings( void) ;
const name_bind *fncmatch( char *name) ;	/* look up by name */

/* bindable functions mapped to prefix keys and hooks */
BINDABLE( nullproc) ;
BINDABLE( metafn) ;
BINDABLE( cex) ;
BINDABLE( unarg) ;

#endif

/* end of names.h */
