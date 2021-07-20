/* ebind.c -- implements ebind.h */
#include "ebind.h"

/*  ebind.c
 *
 *  Initial default key to function bindings
 *
 *  Modified by Petri Kutvonen
 */

#include <stdlib.h>


#include "basic.h"
#include "bind.h"
#include "estruct.h"
#include "bindable.h"
#include "buffer.h"
#include "eval.h"
#include "exec.h"
#include "file.h"
#include "isearch.h"
#include "line.h"
#include "random.h"
#include "region.h"
#include "search.h"
#include "spawn.h"
#include "window.h"
#include "word.h"

/*
 * Command table.
 * This table  is *roughly* in ASCII order, left to right across the
 * characters of the command. This explains the funny location of the
 * control-X commands.
 */
key_tab keytab[ NBINDS] = {
    {CONTROL | '?', backdel, NULL},
    {CONTROL | 'A', (fnp_t) gotobol, NULL}
    ,
    {CONTROL | 'B', (fnp_t) backchar, NULL}
    ,
    {CONTROL | 'C', insspace, NULL}
    ,
    {CONTROL | 'D', forwdel, NULL}
    ,
    {CONTROL | 'E', (fnp_t) gotoeol, NULL}
    ,
    {CONTROL | 'F', (fnp_t) forwchar, NULL}
    ,
    {CONTROL | 'G', ctrlg, NULL}
    ,
    {CONTROL | 'H', backdel, NULL}
    ,
    {CONTROL | 'I', insert_tab, NULL}
    ,
    {CONTROL | 'J', indent, NULL}
    ,
    {CONTROL | 'K', killtext, NULL}
    ,
    {CONTROL | 'L', redraw, NULL}
    ,
    {CONTROL | 'M', insert_newline, NULL}
    ,
    {CONTROL | 'N', (fnp_t) forwline, NULL}
    ,
    {CONTROL | 'O', openline, NULL}
    ,
    {CONTROL | 'P', (fnp_t) backline, NULL}
    ,
    {CONTROL | 'Q', quote, NULL}
    ,
    {CONTROL | 'R', backsearch, NULL}
    ,
    {CONTROL | 'S', forwsearch, NULL}
    ,
    {CONTROL | 'T', (fnp_t) twiddle, NULL}
    ,
    {CONTROL | 'U', unarg, NULL}
    ,
    {CONTROL | 'V', (fnp_t) forwpage, NULL}
    ,
    {CONTROL | 'W', killregion, NULL}
    ,
    {CONTROL | 'X', cex, NULL}
    ,
    {CONTROL | 'Y', yank, NULL}
    ,
    {CONTROL | 'Z', (fnp_t) backpage, NULL}
    ,
    {CONTROL | ']', metafn, NULL}
    ,
    {CTLX | CONTROL | 'B', listbuffers, NULL}
    ,
    {CTLX | CONTROL | 'C', quit, NULL}
    ,           /* Hard quit.           */
#if PKCODE & AEDIT
    {CTLX | CONTROL | 'A', detab, NULL}
    ,
#endif
#if PKCODE
    {CTLX | CONTROL | 'D', filesave, NULL}
    ,           /* alternative          */
#else
#if AEDIT
    {CTLX | CONTROL | 'D', detab, NULL}
    ,
#endif
#endif
#if AEDIT
    {CTLX | CONTROL | 'E', entab, NULL}
    ,
#endif
    {CTLX | CONTROL | 'F', filefind, NULL}
    ,
    {CTLX | CONTROL | 'I', insfile, NULL}
    ,
    {CTLX | CONTROL | 'L', lowerregion, NULL}
    ,
    {CTLX | CONTROL | 'M', delmode, NULL}
    ,
    {CTLX | CONTROL | 'N', mvdnwind, NULL}
    ,
    {CTLX | CONTROL | 'O', deblank, NULL}
    ,
    {CTLX | CONTROL | 'P', mvupwind, NULL}
    ,
    {CTLX | CONTROL | 'R', fileread, NULL}
    ,
    {CTLX | CONTROL | 'S', filesave, NULL}
    ,
#if AEDIT
    {CTLX | CONTROL | 'T', trim, NULL}
    ,
#endif
    {CTLX | CONTROL | 'U', upperregion, NULL}
    ,
    {CTLX | CONTROL | 'V', viewfile, NULL}
    ,
    {CTLX | CONTROL | 'W', filewrite, NULL}
    ,
    {CTLX | CONTROL | 'X', (fnp_t) swapmark, NULL}
    ,
    {CTLX | CONTROL | 'Z', shrinkwind, NULL}
    ,
    {CTLX | '?', deskey, NULL}
    ,
    {CTLX | '!', spawn, NULL}
    ,
    {CTLX | '@', pipecmd, NULL}
    ,
    {CTLX | '#', filter_buffer, NULL}
    ,
    {CTLX | '$', execprg, NULL}
    ,
    {CTLX | '=', showcpos, NULL}
    ,
    {CTLX | '(', ctlxlp, NULL}
    ,
    {CTLX | ')', ctlxrp, NULL}
    ,
    {CTLX | '^', enlargewind, NULL}
    ,
    {CTLX | '0', delwind, NULL}
    ,
    {CTLX | '1', onlywind, NULL}
    ,
    {CTLX | '2', splitwind, NULL}
    ,
    {CTLX | 'A', setvar, NULL}
    ,
    {CTLX | 'B', usebuffer, NULL}
    ,
    {CTLX | 'C', spawncli, NULL}
    ,
#if BSD | SVR4
    {CTLX | 'D', bktoshell, NULL}
    ,
#endif
    {CTLX | 'E', ctlxe, NULL}
    ,
    {CTLX | 'F', setfillcol, NULL}
    ,
    {CTLX | 'K', killbuffer, NULL}
    ,
    {CTLX | 'M', setemode, NULL}
    ,
    {CTLX | 'N', filename, NULL}
    ,
    {CTLX | 'O', nextwind, NULL}
    ,
    {CTLX | 'P', prevwind, NULL}
    ,
#if PKCODE
    {CTLX | 'Q', quote, NULL}
    ,           /* alternative  */
#endif
#if ISRCH
    {CTLX | 'R', risearch, NULL}
    ,
    {CTLX | 'S', fisearch, NULL}
    ,
#endif
    {CTLX | 'W', resize, NULL}
    ,
    {CTLX | 'X', nextbuffer, NULL}
    ,
    {CTLX | 'Z', enlargewind, NULL}
    ,
    {META | CONTROL | '?', delbword, NULL},
#if WORDPRO
    {META | CONTROL | 'C', wordcount, NULL}
    ,
#endif
#if PKCODE
    {META | CONTROL | 'D', newsize, NULL}
    ,
#endif
#if PROC
    {META | CONTROL | 'E', execproc, NULL}
    ,
#endif
#if CFENCE
    {META | CONTROL | 'F', getfence, NULL}
    ,
#endif
    {META | CONTROL | 'H', delbword, NULL}
    ,
    {META | CONTROL | 'K', unbindkey, NULL}
    ,
    {META | CONTROL | 'L', reposition, NULL}
    ,
    {META | CONTROL | 'M', delgmode, NULL}
    ,
    {META | CONTROL | 'N', namebuffer, NULL}
    ,
    {META | CONTROL | 'R', qreplace, NULL}
    ,
    {META | CONTROL | 'S', newsize, NULL}
    ,
    {META | CONTROL | 'T', newwidth, NULL}
    ,
    {META | CONTROL | 'V', scrnextdw, NULL}
    ,
#if WORDPRO
    {META | CONTROL | 'W', killpara, NULL}
    ,
#endif
    {META | CONTROL | 'Z', scrnextup, NULL}
    ,
    {META | ' ', (fnp_t) setmark, NULL}
    ,
    {META | '?', help, NULL}
    ,
    {META | '!', reposition, NULL}
    ,
    {META | '.', (fnp_t) setmark, NULL}
    ,
    {META | '>', (fnp_t) gotoeob, NULL}
    ,
    {META | '<', (fnp_t) gotobob, NULL}
    ,
    {META | '~', unmark, NULL}
    ,
#if APROP
    {META | 'A', apro, NULL}
    ,
#endif
    {META | 'B', backword, NULL}
    ,
    {META | 'C', capword, NULL}
    ,
    {META | 'D', delfword, NULL}
    ,
    {META | 'F', forwword, NULL}
    ,
    {META | 'G', gotoline, NULL}
    ,
#if PKCODE
#if WORDPRO
    {META | 'J', justpara, NULL}
    ,
#endif
#endif
    {META | 'K', bindtokey, NULL}
    ,
    {META | 'L', lowerword, NULL}
    ,
    {META | 'M', setgmode, NULL}
    ,
#if WORDPRO
    {META | 'N', gotoeop, NULL}
    ,
    {META | 'P', gotobop, NULL}
    ,
    {META | 'Q', fillpara, NULL}
    ,
#endif
    {META | 'R', sreplace, NULL}
    ,
#if PKCODE
    {META | 'S', forwhunt, NULL}
    ,
#else
#if BSD
    {META | 'S', bktoshell, NULL}
    ,
#endif
#endif
    {META | 'U', upperword, NULL}
    ,
    {META | 'V', (fnp_t) backpage, NULL}
    ,
    {META | 'W', copyregion, NULL}
    ,
    {META | 'X', namedcmd, NULL}
    ,
    {META | 'Z', quickexit, NULL}
    ,

#if VT220
    {SPEC | '1', (fnp_t) gotobob /* fisearch */, NULL}
    ,           /* VT220 keys   */
    {SPEC | '2', yank, NULL}
    ,
    {SPEC | '3', forwdel /* killregion */, NULL}
    ,
    {SPEC | '4', (fnp_t) gotoeob /* setmark */, NULL}
    ,
    {SPEC | '5', (fnp_t) backpage, NULL}
    ,
    {SPEC | '6', (fnp_t) forwpage, NULL}
    ,
    {SPEC | 'A', (fnp_t) backline, NULL}
    ,
    {SPEC | 'B', (fnp_t) forwline, NULL}
    ,
    {SPEC | 'C', (fnp_t) forwchar, NULL}
    ,
    {SPEC | 'D', (fnp_t) backchar, NULL}
    ,
    {SPEC | 'c', metafn, NULL}
    ,
    {SPEC | 'd', (fnp_t) backchar, NULL}
    ,
    {SPEC | 'e', (fnp_t) forwline, NULL}
    ,
    {SPEC | 'f', (fnp_t) gotobob, NULL}
    ,
    {SPEC | 'h', help, NULL}
    ,
    {SPEC | 'i', cex, NULL}
    ,
#endif

    /* special internal bindings */
    { SPEC | META | 'W', wrapword , NULL},    /* called on word wrap */
    { SPEC | META | 'C', nullproc , NULL},    /*  every command input */
    { SPEC | META | 'R', nullproc , NULL},    /*  on file read */
    { SPEC | META | 'X', nullproc , NULL},    /*  on window change P.K. */

    {0, NULL, NULL}
};
