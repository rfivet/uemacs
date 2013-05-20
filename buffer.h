#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "estruct.h"

int usebuffer( int f, int n) ;
int nextbuffer( int f, int n) ;
int swbuffer( struct buffer *bp) ;
int killbuffer( int f, int n) ;
int zotbuf( struct buffer *bp) ;
int namebuffer( int f, int n) ;
int listbuffers( int f, int n) ;
int makelist( int iflag) ;
void ltoa( char *buf, int width, long num) ;
int addline( char *text) ;
int anycb( void) ;
int bclear( struct buffer *bp) ;
int unmark( int f, int n) ;
/* Lookup a buffer by name. */
struct buffer *bfind( char *bname, int cflag, int bflag) ;

#endif
