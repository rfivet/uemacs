#ifndef _INPUT_H_
#define _INPUT_H_

#include "bind.h"


typedef enum {
	STOP, PLAY, RECORD
} kbdstate ;
extern kbdstate kbdmode ;	/* current keyboard macro mode  */
extern int lastkey ;		/* last keystoke                */
extern int kbdrep ;		/* number of repetitions        */
extern int kbdm[] ;		/* Holds kayboard macro data    */
extern int *kbdptr ;		/* current position in keyboard buf */
extern int *kbdend ;		/* ptr to end of the keyboard	*/
extern int disinp ;		/* display input characters     */

extern int metac;		/* current meta character */
extern int ctlxc;		/* current control X prefix char */
extern int reptc;		/* current universal repeat char */
extern int abortc;		/* current abort command char   */
extern const int nlc ;	/* end of input char */


int mlyesno( const char *prompt) ;
int mlreply( const char *prompt, char *buf, int nbuf) ;
int newmlargt( char **outbufref, const char *prompt, int size) ;
int ectoc( int c) ;
fn_t getname( void) ;
int tgetc( void) ;
int get1key( void) ;
int getcmd( void) ;
int getstring( const char *prompt, char *buf, int nbuf, int eolchar) ;
void ostring( char *s) ;

#endif
