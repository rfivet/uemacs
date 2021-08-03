/* region.h -- a region starts at the mark and end at the dot */
#ifndef _REGION_H_
#define _REGION_H_

#include "line.h"

/*
 * The starting position of a region, and the size of the region in
 * characters, is kept in a region structure.  Used by the region commands.
 */
typedef struct {
	line_p	r_linep ;	/* Origin struct line address.	*/
	int 	r_offset ;	/* Origin struct line offset.   */
	long 	r_size ;	/* Length in characters.        */
} region_t ;

typedef region_t *region_p ;

/* Bindable functions */
BINDABLE( killregion) ;
BINDABLE( copyregion) ;
BINDABLE( lowerregion) ;
BINDABLE( upperregion) ;

int getregion( region_p rp) ;

#endif

/* end of region.h */
