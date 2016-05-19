#ifndef __RETCODE_H__
#define __RETCODE_H__

#ifdef	FALSE
#error "FALSE shouldn't be defined"
#undef	FALSE
#endif
#ifdef	TRUE
#error "TRUE shouldn't be defined"
#undef	TRUE
#endif

typedef enum {
	FALSE,			/* 0, false, no, bad, etc.		*/
	TRUE			/* 1, true, yes, good, etc.		*/
} boolean ;

#define ABORT 2		/* 2, death, ^G, abort, etc.	*/

#endif
