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

int mlyesno( const char *prompt) ;
int mlreply( const char *prompt, char *buf, int nbuf) ;
int mlreplyt( const char *prompt, char *buf, int nbuf, int eolchar) ;
int ectoc( int c) ;
int ctoec( int c) ;
fn_t getname( void) ;
int tgetc( void) ;
int get1key( void) ;
int getcmd( void) ;
int getstring( const char *prompt, char *buf, int nbuf, int eolchar) ;
void outstring( char *s) ;
void ostring( char *s) ;

#endif
