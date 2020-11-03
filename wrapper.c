/* wrapper.c -- implements wrapper.h */

#include "wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static void die( const char *err) {
	fprintf( stderr, "fatal: %s\n", err) ;
	exit( EXIT_FAILURE) ;
}

/* Function copyright: git */
void xmkstemp( char *template) {
	int fd ;
	mode_t o_mask ;

	o_mask = umask( 0177) ;
	fd = mkstemp( template) ;
	if( fd < 0)
		die( "Unable to create temporary file") ;

	umask( o_mask) ;
	close( fd) ;
}

void *xmalloc( size_t size) {
	void *ret = malloc( size) ;
	if( !ret)
		die( "Out of memory") ;

	return ret ;
}

/* end of wrapper.c */
