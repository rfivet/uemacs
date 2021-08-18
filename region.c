/* region.c -- implements region.h */
#include "region.h"

/* The routines in this file deal with the region, that magic space between
   "." and mark.  Some functions are commands.  Some functions are just for
   internal use.

	Modified by Petri Kutvonen
 */

#include <assert.h>
#include <stddef.h>

#include "buffer.h"
#include "defines.h"
#include "line.h"
#include "mlout.h"
#include "random.h"
#include "window.h"

/* Kill the region.  Ask getregion() to figure out the bounds of the
   region.  Move "." to the start, and kill the characters.  Bound to C-W
   kill-region.
 */
BINDABLE( killregion) {
	region_t region ;

	assert( !(curbp->b_mode & MDVIEW)) ;

	int ret = getregion( &region) ;
	if( ret != TRUE)
		return ret ;

	if( (lastflag & CFKILL) == 0)	/* This is a kill type  */
		kdelete() ;	/* command, so do magic */

	thisflag |= CFKILL ;	/* kill buffer stuff.   */
	curwp->w_dotp = region.r_linep ;
	curwp->w_doto = region.r_offset ;
	return ldelete( region.r_size, TRUE) ;
}

/* Copy all of the characters in the region to the kill buffer.  Don't move
   dot at all.  This is a bit like a kill region followed by a yank.  Bound
   to M-W copy-region.
 */
BINDABLE( copyregion) {
	region_t region ;

	int ret = getregion( &region) ;
	if( ret != TRUE)
		return ret ;

	if( (lastflag & CFKILL) == 0)	/* Kill type command.   */
		kdelete() ;

	thisflag |= CFKILL ;
	line_p linep = region.r_linep ;	/* Current line.        */
	int loffs = region.r_offset ;	/* Current offset.      */
	while( region.r_size--) {
		if( loffs == llength( linep)) {	/* End of line.         */
			ret = kinsert( '\n') ;
			if( ret != TRUE)
				return ret ;

			linep = lforw( linep) ;
			loffs = 0 ;
		} else {	/* Middle of line.      */
			ret = kinsert( lgetc( linep, loffs)) ;
			if( ret != TRUE)
				return ret ;

			++loffs ;
		}
	}

	mloutstr( "(region copied)") ;
	return TRUE ;
}

/* Lower case region & Upper case region.  Zap all of the upper/lower case
   characters in the region to lower/upper case.  Use the region code to
   set the limits.  Scan the buffer, doing the changes.  Call "lchange" to
   ensure that redisplay is done in all buffers.  Bound to C-X C-L
   case-region-lower and C-X C-U case-region-upper.
 */
static int utol( int c) {
	return (c >= 'A' && c <= 'Z') ? c + 'a' - 'A' : -1 ;
}

static int ltou( int c) {
	return (c >= 'a' && c <= 'z') ? c + 'A' - 'a' : -1 ;
}

static int caseregion( int (* cconv)( int)) {
	region_t region ;

	assert( !(curbp->b_mode & MDVIEW)) ;
	
	int ret = getregion( &region) ;
	if( ret != TRUE)
		return ret ;

	lchange( WFHARD) ;
	line_p linep = region.r_linep ;
	int loffs = region.r_offset ;
	while( region.r_size--) {
		if( loffs == llength( linep)) {
			linep = lforw( linep) ;
			loffs = 0 ;
		} else {
			int c = cconv( lgetc( linep, loffs)) ;
			if( c != -1)
				lputc( linep, loffs, c) ;

			++loffs ;
		}
	}

	return TRUE ;
}

BINDABLE( lowerregion) {
	return caseregion( utol) ;
}

BINDABLE( upperregion) {
	return caseregion( ltou) ;
}

/* This routine figures out the bounds of the region in the current window,
 * and fills in the fields of the "region_t" structure pointed to by "rp".
 * Because the dot and mark are usually very close together, we scan
 * outward from dot looking for mark.  This should save time.  Return a
 * standard code.  Callers of this routine should be prepared to get an
 * "ABORT" status; we might make this have the conform thing later.
 */
int getregion( region_p rp) {
	line_p flp, blp ;
	long fsize, bsize ;

	if (curwp->w_markp == NULL)
		return mloutfail( "No mark set in this window") ;

	if (curwp->w_dotp == curwp->w_markp) {
		rp->r_linep = curwp->w_dotp;
		if (curwp->w_doto < curwp->w_marko) {
			rp->r_offset = curwp->w_doto;
			rp->r_size =
			    (long) (curwp->w_marko - curwp->w_doto);
		} else {
			rp->r_offset = curwp->w_marko;
			rp->r_size =
			    (long) (curwp->w_doto - curwp->w_marko);
		}
		return TRUE;
	}
	blp = curwp->w_dotp;
	bsize = (long) curwp->w_doto;
	flp = curwp->w_dotp;
	fsize = (long) (llength(flp) - curwp->w_doto + 1);
	while (flp != curbp->b_linep || lback(blp) != curbp->b_linep) {
		if (flp != curbp->b_linep) {
			flp = lforw(flp);
			if (flp == curwp->w_markp) {
				rp->r_linep = curwp->w_dotp;
				rp->r_offset = curwp->w_doto;
				rp->r_size = fsize + curwp->w_marko;
				return TRUE;
			}
			fsize += llength(flp) + 1;
		}
		if (lback(blp) != curbp->b_linep) {
			blp = lback(blp);
			bsize += llength(blp) + 1;
			if (blp == curwp->w_markp) {
				rp->r_linep = blp;
				rp->r_offset = curwp->w_marko;
				rp->r_size = bsize - curwp->w_marko;
				return TRUE;
			}
		}
	}

	return mloutfail( "Bug: lost mark") ;
}

/* end of region.c */
