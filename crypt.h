#ifndef _CRYPT_H_
#define _CRYPT_H_

#define	CRYPT 1	/* file encryption enabled? */

#if CRYPT
void myencrypt( char *bptr, unsigned len) ;
#endif

#endif
