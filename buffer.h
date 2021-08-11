/* buffer.h -- buffer type and functions */
#ifndef _BUFFER_H_
# define _BUFFER_H_

#include "line.h"
#include "names.h"

/* Text is kept in buffers.  A buffer header, described below, exists for
   every buffer in the system.  The buffers are kept in a big list, so that
   commands that search for a buffer by name can find the buffer header.
   There is a safe store for the dot and mark in the header, but this is
   only valid if the buffer is not being displayed (that is, if "b_nwnd" is
   0).  The text for the buffer is kept in a circularly linked list of
   lines, with a pointer to the header line in "b_linep".

   Buffers may be "Inactive" which means the files associated with them
   have not been read in yet.  These get read in at "use buffer" time.
 */

typedef char fname_t[ 256] ;    /* file name type */
typedef char bname_t[ 16] ;     /* buffer name type */

typedef struct buffer {
    struct buffer *b_bufp ; /* Link to next struct buffer           */
    line_p b_dotp ;         /* Link to "." struct line structure    */
    line_p b_markp ;        /* The same as the above two,           */
    line_p b_linep ;        /* Link to the header struct line       */
    int b_doto ;            /* Offset of "." in above struct line   */
    int b_marko ;           /* but for the "mark"           */
    int b_mode ;            /* editor mode of this buffer   */
    char b_active ;         /* window activated flag        */
    char b_nwnd ;           /* Count of windows on buffer   */
    char b_flag ;           /* Flags                        */
    fname_t b_fname ;       /* File name                    */
    bname_t b_bname ;       /* Buffer name                  */
} *buffer_p ;

extern buffer_p curbp ;     /* Current buffer               */
extern buffer_p bheadp ;    /* Head of list of buffers      */
extern buffer_p blistp ;    /* Buffer for C-X C-B           */

#define BFINVS  0x01        /* Internal invisable buffer    */
#define BFCHG   0x02        /* Changed since last write     */
#define BFTRUNC 0x04        /* buffer was truncated when read */

/*  mode flags  */
#define NUMMODES    9       /* # of defined modes           */

#define MDWRAP  0x0001      /* word wrap                    */
#define MDCMOD  0x0002      /* C indentation and fence match */
#define MDEXACT 0x0004      /* Exact matching for searches  */
#define MDVIEW  0x0008      /* read-only buffer             */
#define MDOVER  0x0010      /* overwrite mode               */
#define MDMAGIC 0x0020      /* regular expresions in search */
#define MDASAVE 0x0040      /* auto-save mode               */
#define MDUTF8  0x0080      /* utf8 mode                    */
#define MDDOS   0x0100      /* CRLF eol mode                */

extern const char *modename[ NUMMODES] ;    /* text names of modes */
extern int gmode ;                          /* global editor mode */


/* Bindable functions */
BINDABLE( killbuffer) ;
BINDABLE( listbuffers) ;
BINDABLE( namebuffer) ;
BINDABLE( nextbuffer) ;
BINDABLE( unmark) ;
BINDABLE( usebuffer) ;

boolean anycb( void) ;          /* Any changed buffer? */
int bclear( buffer_p bp) ;      /* empty buffer */
int swbuffer( buffer_p bp) ;    /* switch to buffer, make it current */
int zotbuf( buffer_p bp) ;      /* remove buffer */

/* Lookup a buffer by name.  If not found and create_f is TRUE then create
   it with flags set.
*/
buffer_p bfind( const char *bname, boolean create_f, int flags) ;

#endif
/* end of buffer.h */
