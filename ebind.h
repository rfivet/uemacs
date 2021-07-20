#ifndef _EBIND_H_
#define _EBIND_H_

#include "names.h"

/* Structure for the key bindings table. */
typedef struct key_tab {
	unsigned		k_code ;		/* Key code */
	fnp_t			k_fp ;			/* Routine to handle it */
	const name_bind	*k_nbp ;		/* entry in name to function map table */
} key_tab ;

/* keycode to function mapping table */
#define	NBINDS	256					/* max # of bound keys          */
extern key_tab keytab[ NBINDS] ;	/* key bind to functions table  */

#endif
