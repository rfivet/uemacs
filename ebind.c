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
    {CONTROL | '?', backdel},
    {CONTROL | 'A', (fnp_t) gotobol}
    ,
    {CONTROL | 'B', (fnp_t) backchar}
    ,
    {CONTROL | 'C', insspace}
    ,
    {CONTROL | 'D', forwdel}
    ,
    {CONTROL | 'E', (fnp_t) gotoeol}
    ,
    {CONTROL | 'F', (fnp_t) forwchar}
    ,
    {CONTROL | 'G', ctrlg}
    ,
    {CONTROL | 'H', backdel}
    ,
    {CONTROL | 'I', insert_tab}
    ,
    {CONTROL | 'J', indent}
    ,
    {CONTROL | 'K', killtext}
    ,
    {CONTROL | 'L', redraw}
    ,
    {CONTROL | 'M', insert_newline}
    ,
    {CONTROL | 'N', (fnp_t) forwline}
    ,
    {CONTROL | 'O', openline}
    ,
    {CONTROL | 'P', (fnp_t) backline}
    ,
    {CONTROL | 'Q', quote}
    ,
    {CONTROL | 'R', backsearch}
    ,
    {CONTROL | 'S', forwsearch}
    ,
    {CONTROL | 'T', (fnp_t) twiddle}
    ,
    {CONTROL | 'U', unarg}
    ,
    {CONTROL | 'V', (fnp_t) forwpage}
    ,
    {CONTROL | 'W', killregion}
    ,
    {CONTROL | 'X', cex}
    ,
    {CONTROL | 'Y', yank}
    ,
    {CONTROL | 'Z', (fnp_t) backpage}
    ,
    {CONTROL | ']', metafn}
    ,
    {CTLX | CONTROL | 'B', listbuffers}
    ,
    {CTLX | CONTROL | 'C', quit}
    ,           /* Hard quit.           */
#if PKCODE & AEDIT
    {CTLX | CONTROL | 'A', detab}
    ,
#endif
#if PKCODE
    {CTLX | CONTROL | 'D', filesave}
    ,           /* alternative          */
#else
#if AEDIT
    {CTLX | CONTROL | 'D', detab}
    ,
#endif
#endif
#if AEDIT
    {CTLX | CONTROL | 'E', entab}
    ,
#endif
    {CTLX | CONTROL | 'F', filefind}
    ,
    {CTLX | CONTROL | 'I', insfile}
    ,
    {CTLX | CONTROL | 'L', lowerregion}
    ,
    {CTLX | CONTROL | 'M', delmode}
    ,
    {CTLX | CONTROL | 'N', mvdnwind}
    ,
    {CTLX | CONTROL | 'O', deblank}
    ,
    {CTLX | CONTROL | 'P', mvupwind}
    ,
    {CTLX | CONTROL | 'R', fileread}
    ,
    {CTLX | CONTROL | 'S', filesave}
    ,
#if AEDIT
    {CTLX | CONTROL | 'T', trim}
    ,
#endif
    {CTLX | CONTROL | 'U', upperregion}
    ,
    {CTLX | CONTROL | 'V', viewfile}
    ,
    {CTLX | CONTROL | 'W', filewrite}
    ,
    {CTLX | CONTROL | 'X', (fnp_t) swapmark}
    ,
    {CTLX | CONTROL | 'Z', shrinkwind}
    ,
    {CTLX | '?', deskey}
    ,
    {CTLX | '!', spawn}
    ,
    {CTLX | '@', pipecmd}
    ,
    {CTLX | '#', filter_buffer}
    ,
    {CTLX | '$', execprg}
    ,
    {CTLX | '=', showcpos}
    ,
    {CTLX | '(', ctlxlp}
    ,
    {CTLX | ')', ctlxrp}
    ,
    {CTLX | '^', enlargewind}
    ,
    {CTLX | '0', delwind}
    ,
    {CTLX | '1', onlywind}
    ,
    {CTLX | '2', splitwind}
    ,
    {CTLX | 'A', setvar}
    ,
    {CTLX | 'B', usebuffer}
    ,
    {CTLX | 'C', spawncli}
    ,
#if BSD | SVR4
    {CTLX | 'D', bktoshell}
    ,
#endif
    {CTLX | 'E', ctlxe}
    ,
    {CTLX | 'F', setfillcol}
    ,
    {CTLX | 'K', killbuffer}
    ,
    {CTLX | 'M', setemode}
    ,
    {CTLX | 'N', filename}
    ,
    {CTLX | 'O', nextwind}
    ,
    {CTLX | 'P', prevwind}
    ,
#if PKCODE
    {CTLX | 'Q', quote}
    ,           /* alternative  */
#endif
#if ISRCH
    {CTLX | 'R', risearch}
    ,
    {CTLX | 'S', fisearch}
    ,
#endif
    {CTLX | 'W', resize}
    ,
    {CTLX | 'X', nextbuffer}
    ,
    {CTLX | 'Z', enlargewind}
    ,
    {META | CONTROL | '?', delbword},
#if WORDPRO
    {META | CONTROL | 'C', wordcount}
    ,
#endif
#if PKCODE
    {META | CONTROL | 'D', newsize}
    ,
#endif
#if PROC
    {META | CONTROL | 'E', execproc}
    ,
#endif
#if CFENCE
    {META | CONTROL | 'F', getfence}
    ,
#endif
    {META | CONTROL | 'H', delbword}
    ,
    {META | CONTROL | 'K', unbindkey}
    ,
    {META | CONTROL | 'L', reposition}
    ,
    {META | CONTROL | 'M', delgmode}
    ,
    {META | CONTROL | 'N', namebuffer}
    ,
    {META | CONTROL | 'R', qreplace}
    ,
    {META | CONTROL | 'S', newsize}
    ,
    {META | CONTROL | 'T', newwidth}
    ,
    {META | CONTROL | 'V', scrnextdw}
    ,
#if WORDPRO
    {META | CONTROL | 'W', killpara}
    ,
#endif
    {META | CONTROL | 'Z', scrnextup}
    ,
    {META | ' ', (fnp_t) setmark}
    ,
    {META | '?', help}
    ,
    {META | '!', reposition}
    ,
    {META | '.', (fnp_t) setmark}
    ,
    {META | '>', (fnp_t) gotoeob}
    ,
    {META | '<', (fnp_t) gotobob}
    ,
    {META | '~', unmark}
    ,
#if APROP
    {META | 'A', apro}
    ,
#endif
    {META | 'B', backword}
    ,
    {META | 'C', capword}
    ,
    {META | 'D', delfword}
    ,
    {META | 'F', forwword}
    ,
    {META | 'G', gotoline}
    ,
#if PKCODE
#if WORDPRO
    {META | 'J', justpara}
    ,
#endif
#endif
    {META | 'K', bindtokey}
    ,
    {META | 'L', lowerword}
    ,
    {META | 'M', setgmode}
    ,
#if WORDPRO
    {META | 'N', gotoeop}
    ,
    {META | 'P', gotobop}
    ,
    {META | 'Q', fillpara}
    ,
#endif
    {META | 'R', sreplace}
    ,
#if PKCODE
    {META | 'S', forwhunt}
    ,
#else
#if BSD
    {META | 'S', bktoshell}
    ,
#endif
#endif
    {META | 'U', upperword}
    ,
    {META | 'V', (fnp_t) backpage}
    ,
    {META | 'W', copyregion}
    ,
    {META | 'X', namedcmd}
    ,
    {META | 'Z', quickexit}
    ,

#if VT220
    {SPEC | '1', (fnp_t) gotobob /* fisearch */}
    ,           /* VT220 keys   */
    {SPEC | '2', yank}
    ,
    {SPEC | '3', forwdel /* killregion */}
    ,
    {SPEC | '4', (fnp_t) gotoeob /* setmark */}
    ,
    {SPEC | '5', (fnp_t) backpage}
    ,
    {SPEC | '6', (fnp_t) forwpage}
    ,
    {SPEC | 'A', (fnp_t) backline}
    ,
    {SPEC | 'B', (fnp_t) forwline}
    ,
    {SPEC | 'C', (fnp_t) forwchar}
    ,
    {SPEC | 'D', (fnp_t) backchar}
    ,
    {SPEC | 'c', metafn}
    ,
    {SPEC | 'd', (fnp_t) backchar}
    ,
    {SPEC | 'e', (fnp_t) forwline}
    ,
    {SPEC | 'f', (fnp_t) gotobob}
    ,
    {SPEC | 'h', help}
    ,
    {SPEC | 'i', cex}
    ,
#endif

    /* special internal bindings */
    { SPEC | META | 'W', wrapword },    /* called on word wrap */
    { SPEC | META | 'C', nullproc },    /*  every command input */
    { SPEC | META | 'R', nullproc },    /*  on file read */
    { SPEC | META | 'X', nullproc },    /*  on window change P.K. */

    {0, NULL}
};
