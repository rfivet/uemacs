#include "log.h"

#include "retcode.h"


static void logdump( const char *buf, ...) {
}

void (*logwrite)( const char *, ...) = logdump ;

static int logit( int retcode, int beep_f, const char *buf, ...) {
	return retcode ;
}

int (*logger)( int, int, const char *, ...) = logit ;

/*
 * tell the user that this command is illegal while we are in
 * VIEW (read-only) mode
 */
int rdonly(void)
{
/*	TTbeep();
	mlwrite("(Key illegal in VIEW mode)");
	return FALSE;
*/
	return logger( FALSE, TRUE, "(Key illegal in VIEW mode)");
}



int resterr(void)
{
/*	TTbeep();
	mlwrite("(That command is RESTRICTED)");
	return FALSE;
*/
	return logger( FALSE, TRUE, "(That command is RESTRICTED)");
}


