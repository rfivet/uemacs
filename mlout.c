/* mlout.c -- implements mlout.h */

#include "mlout.h"

/* by default just use a stub */
static void mloutdump( const char *buf, ...) {
}

void (*mloutfmt)( const char *, ...) = mloutdump ;

void mloutstr( const char *str) {
	mloutfmt( (*str) ? "%s" : "", str) ;
}

boolean mloutfail( const char *msg) {
	mloutfmt( "%B%s", msg) ;
	return FALSE ;
}

/* end of mlout.c */
