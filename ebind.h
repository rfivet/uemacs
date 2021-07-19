#ifndef _EBIND_H_
#define _EBIND_H_

#include "fnp_t.h"

/* Structure for the table of initial key bindings. */
typedef struct key_tab {
	unsigned	k_code ;			/* Key code */
	fnp_t		k_fp ;				/* Routine to handle it */
} key_tab ;

#define	NBINDS	256					/* max # of bound keys          */
extern key_tab keytab[ NBINDS] ;	/* key bind to functions table  */

#endif
