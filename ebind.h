/* Structure for the table of initial key bindings. */
struct key_tab {
	int k_code ;			 /* Key code */
	int (*k_fp)( int, int) ;	 /* Routine to handle it */
} ;

#define	NBINDS	256			/* max # of bound keys          */
extern struct key_tab keytab[ NBINDS] ;	/* key bind to functions table  */

