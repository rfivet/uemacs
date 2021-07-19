#ifndef _NAMES_H_
#define _NAMES_H_

#include "fnp_t.h"

/* Structure for the name binding table. */
typedef struct name_bind {
	const char	*n_name ;			/* name of function key */
	fnp_t	n_func ;				/* function name is bound to */
	char	tag ;					/* view mode compatibility tag */
} name_bind ;

extern const name_bind names[] ;	/* name to function table */

#endif
