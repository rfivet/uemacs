#include "log.h"

void logdump( const char *buf, ...) {
}

void (*logwrite)( const char *, ...) = logdump ;
