/* wrapper.c -- implements wrapper.h */

#include "wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef MINGW32
int mkstemp( char *template) {
	return -1 ;
}
#endif

static void die( const char *err) {
	fprintf( stderr, "fatal: %s\n", err) ;
	exit( EXIT_FAILURE) ;
}

/* Function copyright: git */
void xmkstemp( char *template) {
	int fd ;

	fd = mkstemp( template) ;
	if( fd < 0)
		die( "Unable to create temporary file") ;

	close( fd) ;
}

void *xmalloc( size_t size) {
	void *ret = malloc( size) ;
	if( !ret)
		die( "Out of memory") ;

	return ret ;
}

/* end of wrapper.c */
