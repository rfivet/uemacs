/* window.h -- window functionality */
#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "defines.h"    /* COLOR, SCROLLCODE */
#include "buffer.h"     /* buffer_p, line_p */
#include "names.h"      /* BINDABLE() */

/* There is a window structure allocated for every active display window.
   The windows are kept in a big list, in top to bottom screen order, with
   the listhead at "wheadp".  Each window contains its own values of dot
   and mark.  The flag field contains some bits that are set by commands to
   guide redisplay.  Although this is a bit of a compromise in terms of
   decoupling, the full blown redisplay is just too expensive to run for
   every input character.
 */
typedef struct window {
    struct window *w_wndp;  /* Next window                  */
    buffer_p w_bufp ;       /* Buffer displayed in window   */
    line_p  w_linep ;       /* Top line in the window       */
    line_p  w_dotp ;        /* Line containing "."          */
    line_p  w_markp ;       /* Line containing "mark"       */
    int w_doto ;            /* Byte offset for "."          */
    int w_marko ;           /* Byte offset for "mark"       */
    int w_toprow ;          /* Origin 0 top row of window   */
    int w_ntrows ;          /* # of rows of text in window  */
    char w_force ;          /* If NZ, forcing row.          */
    char w_flag ;           /* Flags.                       */
#if COLOR
    char w_fcolor ;         /* current forground color      */
    char w_bcolor ;         /* current background color     */
#endif
} *window_p ;

extern window_p curwp ;     /* Current window               */
extern window_p wheadp ;    /* Head of list of windows      */

/* curwbyte return the byte after the dot in current window */
#define curwbyte() lgetc( curwp->w_dotp, curwp->w_doto)

#define WFFORCE 0x01        /* Window needs forced reframe  */
#define WFMOVE  0x02        /* Movement from line to line   */
#define WFEDIT  0x04        /* Editing within a line        */
#define WFHARD  0x08        /* Better to a full display     */
#define WFMODE  0x10        /* Update mode line.            */
#define WFCOLR  0x20        /* Needs a color change         */

#if SCROLLCODE
# define WFKILLS 0x40       /* something was deleted        */
# define WFINS   0x80       /* something was inserted       */
#endif

/* Bindable functions */
 BINDABLE( delwind) ;
 BINDABLE( enlargewind) ;
 BINDABLE( mvdnwind) ;
 BINDABLE( mvupwind) ;
BBINDABLE( newsize) ;
BBINDABLE( newwidth) ;
 BINDABLE( nextwind) ;
 BINDABLE( onlywind) ;
 BINDABLE( prevwind) ;
TBINDABLE( redraw) ;
TBINDABLE( reposition) ;
 BINDABLE( resize) ;
 BINDABLE( restwnd) ;
 BINDABLE( savewnd) ;
 BINDABLE( scrnextdw) ;
 BINDABLE( scrnextup) ;
 BINDABLE( shrinkwind) ;
 BINDABLE( splitwind) ;

int getwpos( void) ;
void cknewwindow( void) ;
window_p wpopup( void) ;  /* Pop up window creation. */

#endif
/* end of window.h */
