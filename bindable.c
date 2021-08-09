/* bindable.c -- implements bindable.h */
#include "bindable.h"

#include <stdlib.h>

#include "defines.h"
#include "buffer.h"
#include "display.h"    /* vttidy() */
#include "file.h"
#include "input.h"
#include "lock.h"
#include "mlout.h"
#include "terminal.h"


/* Fancy quit command, as implemented by Norm.  If any buffer has changed
   do a write on that buffer and exit emacs, otherwise simply exit.
 */
BINDABLE( quickexit) {
    buffer_p oldcb = curbp ;    /* save in case we fail */
    for( buffer_p bp = bheadp ; bp != NULL ; bp = bp->b_bufp) {
        if( (bp->b_flag & (BFCHG | BFTRUNC | BFINVS)) == BFCHG) {
        /* Changed, Not truncated and real buffer */
            curbp = bp ;    /* make that buffer cur */
            mloutfmt( "(Saving %s)", bp->b_fname) ;
            int status = filesave( f, n) ;
            if( status != TRUE) {
                curbp = oldcb ; /* restore curbp */
                return status ;
            }
        }
    }

    return quit( f, n) ;    /* conditionally quit   */
}


/* Quit command. If an argument, always quit. Otherwise confirm if a buffer
 * has been changed and not written out. Normally bound to "C-X C-C".
 */
BINDABLE( quit) {
    int s ; /* status of user query */

    if( f != FALSE      /* Argument forces it.  */
    || anycb() == FALSE /* All buffers clean.   */
                        /* User says it's OK.   */
    || (s = mlyesno( "Modified buffers exist. Leave anyway")) == TRUE) {
#if (FILOCK && BSD) || SVR4
        if( lockrel() != TRUE) {
            TTputc('\n') ;
            TTputc('\r') ;
            TTclose() ;
            TTkclose() ;
            exit( EXIT_FAILURE) ;
        }
#endif
        vttidy() ;
        if( f)
            exit( n) ;
        else
            exit( EXIT_SUCCESS) ;
    }

    mloutstr( "") ;
    return s ;
}


/* Begin a keyboard macro.
 * Error if not at the top level in keyboard processing. Set up variables and
 * return.
 */
BINDABLE( ctlxlp) {
    if( kbdmode != STOP)
        return mloutfail( "%Macro already active") ;

    mloutstr( "(Start macro)") ;
    kbdptr = kbdm ;
    kbdend = kbdptr ;
    kbdmode = RECORD ;
    return TRUE ;
}


/* End keyboard macro. Check for the same limit conditions as the above
 * routine. Set up the variables and return to the caller.
 */
BINDABLE( ctlxrp) {
    if( kbdmode == STOP)
        return mloutfail( "%Macro not active") ;

    if (kbdmode == RECORD) {
        mloutstr( "(End macro)") ;
        kbdmode = STOP;
    }

    return TRUE ;
}


/* Execute a macro.
 * The command argument is the number of times to loop. Quit as soon as a
 * command gets an error. Return TRUE if all ok, else FALSE.
 */
BINDABLE( ctlxe) {
    if( kbdmode != STOP)
        return mloutfail( "%Macro already active") ;

    if( n <= 0)
        return TRUE ;

    kbdrep = n ;        /* remember how many times to execute */
    kbdmode = PLAY ;    /* start us in play mode */
    kbdptr = kbdm ;     /*    at the beginning */
    return TRUE ;
}


/* abort:
 *  Beep the beeper. Kill off any keyboard macro, etc., that is in progress.
 *  Sometimes called as a routine, to do general aborting of stuff.
 */
BINDABLE( ctrlg) {
    kbdmode = STOP ;
    mloutfmt( "%B(Aborted)") ;
    return ABORT ;
}

/* end of bindable.c */
