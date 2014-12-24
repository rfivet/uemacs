#ifndef _REGION_H_
#define _REGION_H_

#include "line.h"

/*
 * The starting position of a region, and the size of the region in
 * characters, is kept in a region structure.  Used by the region commands.
 */
struct region {
	struct line *r_linep;	/* Origin struct line address.         */
	int r_offset;		/* Origin struct line offset.          */
	long r_size;		/* Length in characters.        */
};

int killregion( int f, int n) ;
int copyregion( int f, int n) ;
int lowerregion( int f, int n) ;
int upperregion( int f, int n) ;
int getregion( struct region *rp) ;

#endif
