#include "log.h"

static void logdump( const char *buf, ...) {
}

void (*writefmt)( const char *, ...) = logdump ;

void writestr( const char *str) {
	writefmt( "%s", str) ;
}


/*
 * tell the user that this command is illegal while we are in
 * VIEW (read-only) mode
 */
boolean rdonly( void) {
	writefmt( "%B(Key illegal in VIEW mode)") ;
	return FALSE ;
}
