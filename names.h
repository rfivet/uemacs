/* names.h -- mapping of functions to names and keys */
#ifndef _NAMES_H_
#define _NAMES_H_

#include "retcode.h"

#define CTL_        0x01000000  /* Control flag, or'ed in       */
#define META        0x02000000  /* Meta flag, or'ed in          */
#define CTLX        0x04000000  /* ^X flag, or'ed in            */
#define SPEC        0x08000000  /* special key (function keys)  */
#define PRFXMASK    0x0F000000  /* prefix mask */


/* Bindable uEMACS function pointer type and definition template */
#define  BINDABLE( fname)    int fname( boolean f, int n)
#define BBINDABLE( fname)    boolean fname( boolean f, int n)
#define TBINDABLE   BBINDABLE

typedef BINDABLE( (*fnp_t)) ;


/* Structure for the name binding table. */
typedef struct {
    const char  *n_name ;   /* name starting with one tag character */
    fnp_t       n_func ;    /* function the name is bound to */
    unsigned    n_keycode ; /* default key assignment, 0 when none */
} name_bind ;

typedef const name_bind *nbind_p ;

#define bind_name( p) (&( p)->n_name[ 1])
#define bind_tag( p)  ( p)->n_name[ 0]


/* Structure for the key bindings table. */
typedef struct {
    unsigned    k_code ;            /* Key code */
    nbind_p     k_nbp ;             /* entry in name to function map table */
} *kbind_p ;


extern const name_bind  names[] ;   /* name to function mapping table */
extern kbind_p          keytab ;    /* key bind to functions table  */


boolean init_bindings( void) ;
kbind_p setkeybinding( unsigned key, nbind_p nbp) ;
boolean delkeybinding( unsigned key) ;
kbind_p getkeybinding( unsigned key) ;  /* look up by key code */

/* find a name to function association in the name to function mapping table */
nbind_p fncmatch( char *name) ;     /* look up by name */


/* bindable functions mapped to prefix keys and hooks */
TBINDABLE( nullproc) ;
TBINDABLE( metafn) ;
TBINDABLE( cex) ;
TBINDABLE( unarg) ;

#endif
/* end of names.h */
