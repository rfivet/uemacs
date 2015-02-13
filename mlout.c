/* mlout.c -- implements mlout.h */

#include "mlout.h"

/* by default just use a stub */
static void mloutdump( const char *buf, ...) {
}

void (*mloutfmt)( const char *, ...) = mloutdump ;

void mloutstr( const char *str) {
	mloutfmt( "%s", str) ;
}

/* end of mlout.c */
