#ifndef __RETCODE_H__
#define __RETCODE_H__

#ifdef	FALSE
#undef	FALSE
#endif
#ifdef	TRUE
#undef	TRUE
#endif

#if 0
#define FALSE   0		/* False, no, bad, etc.         */
#define TRUE    1		/* True, yes, good, etc.        */
#define ABORT   2		/* Death, ^G, abort, etc.       */
#define	FAILED	3		/* not-quite fatal false return */
#endif

typedef enum {
	FALSE,
	TRUE
} boolean ;

#define ABORT 2

#endif
