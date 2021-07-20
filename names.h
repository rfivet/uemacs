#ifndef _NAMES_H_
#define _NAMES_H_

/* Generic uEMACS function pointer type */
typedef int (*fnp_t)( int, int) ;

/* Structure for the name binding table. */
typedef struct name_bind {
	const char	*n_name ;	/* name starting with one tag character */
	fnp_t	n_func ;		/* function the name is bound to */
} name_bind ;

#define bind_name( p) (&(p)->n_name[ 1])
#define bind_tag( p)  (p)->n_name[ 0]

extern const name_bind names[] ;	/* name to function mapping table */

#endif
