#include "log.h"

static void logdump( const char *buf, ...) {
}

void (*logwrite)( const char *, ...) = logdump ;

static boolean logit( boolean retcode, boolean beep_f, const char *buf, ...) {
	return retcode ;
}

boolean (*logger)( boolean, boolean, const char *, ...) = logit ;

/*
 * tell the user that this command is illegal while we are in
 * VIEW (read-only) mode
 */
boolean rdonly(void)
{
/*	TTbeep();
	mlwrite("(Key illegal in VIEW mode)");
	return FALSE;
*/
	return logger( FALSE, TRUE, "(Key illegal in VIEW mode)");
}



boolean resterr(void)
{
/*	TTbeep();
	mlwrite("(That command is RESTRICTED)");
	return FALSE;
*/
	return logger( FALSE, TRUE, "(That command is RESTRICTED)");
}


