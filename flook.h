#include "retcode.h"


#define rcfname		pathname[ 0]
#define hlpfname	pathname[ 1]

extern const char *pathname[] ;


boolean fexist( const char *fname) ;
char *flook( const char *fname, int hflag) ;

