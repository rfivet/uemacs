#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "crypt.h"
#include "line.h"

typedef char fname_t[ 80] ;	/* file name type */
typedef char bname_t[ 16] ;	/* buffer name type */
#define NBUFN sizeof( bname_t)

#if CRYPT
typedef char ekey_t[ 128] ; /* encryption key type */
#endif

/*
 * Text is kept in buffers. A buffer header, described below, exists for every
 * buffer in the system. The buffers are kept in a big list, so that commands
 * that search for a buffer by name can find the buffer header. There is a
 * safe store for the dot and mark in the header, but this is only valid if
 * the buffer is not being displayed (that is, if "b_nwnd" is 0). The text for
 * the buffer is kept in a circularly linked list of lines, with a pointer to
 * the header line in "b_linep".
 * 	Buffers may be "Inactive" which means the files associated with them
 * have not been read in yet. These get read in at "use buffer" time.
 */
struct buffer {
    struct buffer *b_bufp;	/* Link to next struct buffer   */
	struct line *b_dotp;	/* Link to "." struct line structure   */
	struct line *b_markp;	/* The same as the above two,   */
	struct line *b_linep;	/* Link to the header struct line      */
	int b_doto;		/* Offset of "." in above struct line  */
	int b_marko;		/* but for the "mark"           */
	int b_mode;		/* editor mode of this buffer   */
	char b_active;		/* window activated flag        */
	char b_nwnd;		/* Count of windows on buffer   */
	char b_flag;		/* Flags                        */
	fname_t b_fname ;	/* File name                    */
	bname_t b_bname ;	/* Buffer name                  */
#if CRYPT
	ekey_t	b_key ;		/* current encrypted key        */
#endif
};

extern struct buffer *curbp ;	/* Current buffer               */
extern struct buffer *bheadp ;	/* Head of list of buffers      */
extern struct buffer *blistp ;	/* Buffer for C-X C-B           */

#define BFINVS  0x01		/* Internal invisable buffer    */
#define BFCHG   0x02		/* Changed since last write     */
#define	BFTRUNC	0x04		/* buffer was truncated when read */

/*	mode flags	*/
#define	NUMMODES	11	/* # of defined modes           */

#define	MDWRAP	0x0001		/* word wrap                    */
#define	MDCMOD	0x0002		/* C indentation and fence match */
#define	MDSPELL	0x0004		/* spell error parcing          */
#define	MDEXACT	0x0008		/* Exact matching for searches  */
#define	MDVIEW	0x0010		/* read-only buffer             */
#define MDOVER	0x0020		/* overwrite mode               */
#define MDMAGIC	0x0040		/* regular expresions in search */
#if CRYPT
#define	MDCRYPT	0x0080		/* encrytion mode active        */
#endif
#define	MDASAVE	0x0100		/* auto-save mode               */
#define MDUTF8	0x0200		/* utf8 mode                    */
#define MDDOS	0x0400		/* CRLF eol mode                */


extern const char *modename[] ;	/* text names of modes          */
extern int gmode ;		/* global editor mode           */


int usebuffer( int f, int n) ;
int nextbuffer( int f, int n) ;
int swbuffer( struct buffer *bp) ;
int killbuffer( int f, int n) ;
int zotbuf( struct buffer *bp) ;
int namebuffer( int f, int n) ;
int listbuffers( int f, int n) ;
int anycb( void) ;
int bclear( struct buffer *bp) ;
int unmark( int f, int n) ;
/* Lookup a buffer by name. */
struct buffer *bfind( const char *bname, int cflag, int bflag) ;

#endif
