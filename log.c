#include "log.h"

static void logdump( const char *buf, ...) {
}

void (*writefmt)( const char *, ...) = logdump ;

void writestr( const char *str) {
	writefmt( "%s", str) ;
}
