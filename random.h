/* random.h -- various commands */

#ifndef _RANDOM_H_
#define _RANDOM_H_

#include "names.h"

/* Command flags */
#define CFCPCN  0x0001		/* Flag that last command was C-P, C-N	*/
#define CFKILL  0x0002		/* Flag that last command was a kill    */

extern int thisflag ;		/* Flags, this command */
extern int lastflag ;		/* Flags, last command */

extern int fillcol ;		/* Fill column */
extern boolean	hardtab ;	/* Use hard tab instead of soft tab */


int getcline( void) ;
int getccol( int bflg) ;
boolean setccol( int pos) ;

/* Bindable functions */
BINDABLE( setfillcol) ;
BINDABLE( showcpos) ;
boolean twiddle( int f, int n) ;
BINDABLE( quote) ;
BINDABLE( insert_tab) ;
BINDABLE( detab) ;
BINDABLE( entab) ;
BINDABLE( trim) ;
BINDABLE( openline) ;
BINDABLE( insert_newline) ;
BINDABLE( deblank) ;
BINDABLE( indent) ;
BINDABLE( forwdel) ;
BINDABLE( backdel) ;
BINDABLE( killtext) ;
BINDABLE( setemode) ;
BINDABLE( delmode) ;
BINDABLE( setgmode) ;
BINDABLE( delgmode) ;
BINDABLE( istring) ;
BINDABLE( ovstring) ;

#endif

/* end of random.h */
