/* list.h -- generic list deletion */
/* Copyright © 2021 Renaud Fivet */
#ifndef _LIST_H_
#define _LIST_H_

typedef struct list {
	struct list *next ;
} *list_p ;

void freelist( list_p lp) ;

#endif
/* end of list.h */
