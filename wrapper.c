/* wrapper.c -- implements wrapper.h */

#include "wrapper.h"

#include "usage.h"

#include <stdlib.h>
#include <unistd.h>

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
