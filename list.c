/* list.c -- implements list.h */
/* Copyright © 2021 Renaud Fivet */
#include "list.h"

#include <stdlib.h> /* free() */

/* free a list */
void freelist( list_p lp) {
	while( lp) {
		list_p next = lp->next ;
		free( lp) ;
		lp = next ;
	}
}

/* end of list.c */
