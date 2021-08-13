/* input.h -- */
#ifndef _INPUT_H_
#define _INPUT_H_

#include "names.h"  /* nbind_p */

typedef enum {
    STOP, PLAY, RECORD
} kbdstate ;


extern kbdstate kbdmode ;   /* current keyboard macro mode  */
extern int lastkey ;        /* last keystoke                */
extern int kbdrep ;         /* number of repetitions        */
extern int kbdm[] ;         /* Holds kayboard macro data    */
extern int *kbdptr ;        /* current position in keyboard buf */
extern int *kbdend ;        /* ptr to end of the keyboard   */

extern int metac ;          /* current meta character */
extern int ctlxc ;          /* current control X prefix char */
extern int reptc ;          /* current universal repeat char */
extern int abortc ;         /* current abort command char   */
extern const int nlc ;      /* end of input char */


void ue_system( const char *cmd) ;
int mlyesno( const char *prompt) ;
int newmlarg( char **outbufref, const char *prompt, int size) ;
int newmlargt( char **outbufref, const char *prompt, int size) ;
int ectoc( int c) ;

/* Get a command binding from the command line or interactively */
nbind_p getname( void) ;

int tgetc( void) ;
int get1key( void) ;
int getcmd( void) ;
int getstring( const char *prompt, char *buf, int nbuf, int eolchar) ;

#endif
/* end of input.h */
