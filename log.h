#include "retcode.h"

boolean rdonly( void) ;
boolean resterr( void) ;

extern void (*logwrite)( const char *, ...) ;
extern boolean (*logger)( boolean, const char *, ...) ;

