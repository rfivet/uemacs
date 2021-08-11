/* util.c -- implements util.h */
#include "util.h"

/* Safe zeroing, no complaining about overlap */
void mystrscpy( char *dst, const char *src, int size) {
	if( size <= 0)
		return ;

	while( --size) {
		char c = *src++ ;
		if( !c)
			break ;

		*dst++ = c ;
	}

	*dst = 0 ;
}

/* end of util.c */
