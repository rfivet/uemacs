/* names.c -- implements names.h */
#include "names.h"

/* Name to function binding table.
 *
 * This table gives the names of all the bindable functions
 * and their C function address. These are used for the bind-to-key
 * function.
*/

#include "basic.h"
#include "bind.h"
#include "bindable.h"
#include "buffer.h"
#include "display.h"
#include "eval.h"
#include "exec.h"
#include "file.h"
#include "isearch.h"
#include "line.h"
#include "region.h"
#include "random.h"
#include "search.h"
#include "spawn.h"
#include "window.h"
#include "word.h"

const name_bind names[] = {
	{"abort-command", ctrlg, 0} ,
	{"add-mode", setemode, 0} ,
	{"add-global-mode", setgmode, 0} ,
#if	APROP
	{"apropos", apro, 0} ,
#endif
	{"backward-character", (fnp_t) backchar, 0} ,
	{"begin-macro", ctlxlp, 0} ,
	{"beginning-of-file", (fnp_t) gotobob, 0} ,
	{"beginning-of-line", (fnp_t) gotobol, 0} ,
	{"bind-to-key", bindtokey, 0} ,
	{"buffer-position", showcpos, 0} ,
	{"case-region-lower", lowerregion, 1} ,
	{"case-region-upper", upperregion, 1} ,
	{"case-word-capitalize", capword, 1} ,
	{"case-word-lower", lowerword, 1} ,
	{"case-word-upper", upperword, 1} ,
	{"change-file-name", filename, 0} ,
	{"change-screen-size", newsize, 0} ,
	{"change-screen-width", newwidth, 0} ,
	{"clear-and-redraw", redraw, 0} ,
	{"clear-message-line", clrmes, 0} ,
	{"copy-region", copyregion, 0} ,
#if	WORDPRO
	{"count-words", wordcount, 0} ,
#endif
	{"ctlx-prefix", cex, 0} ,
	{"delete-blank-lines", deblank, 1} ,
	{"delete-buffer", killbuffer, 0} ,
	{"delete-mode", delmode, 0} ,
	{"delete-global-mode", delgmode, 0} ,
	{"delete-next-character", forwdel, 1} ,
	{"delete-next-word", delfword, 1} ,
	{"delete-other-windows", onlywind, 0} ,
	{"delete-previous-character", backdel, 1} ,
	{"delete-previous-word", delbword, 1} ,
	{"delete-window", delwind, 0} ,
	{"describe-bindings", desbind, 0} ,
	{"describe-key", deskey, 0} ,
#if	AEDIT
	{"detab-line", detab, 1} ,
#endif
	{"end-macro", ctlxrp, 0} ,
	{"end-of-file", (fnp_t) gotoeob, 0} ,
	{"end-of-line", (fnp_t) gotoeol, 0} ,
#if	AEDIT
	{"entab-line", entab, 1} ,
#endif
	{"exchange-point-and-mark", (fnp_t) swapmark, 0} ,
	{"execute-buffer", execbuf, 0} ,
	{"execute-command-line", execcmd, 0} ,
	{"execute-file", execfile, 0} ,
	{"execute-macro", ctlxe, 0} ,
	{"execute-macro-1", cbuf1, 0} ,
	{"execute-macro-2", cbuf2, 0} ,
	{"execute-macro-3", cbuf3, 0} ,
	{"execute-macro-4", cbuf4, 0} ,
	{"execute-macro-5", cbuf5, 0} ,
	{"execute-macro-6", cbuf6, 0} ,
	{"execute-macro-7", cbuf7, 0} ,
	{"execute-macro-8", cbuf8, 0} ,
	{"execute-macro-9", cbuf9, 0} ,
	{"execute-macro-10", cbuf10, 0} ,
	{"execute-macro-11", cbuf11, 0} ,
	{"execute-macro-12", cbuf12, 0} ,
	{"execute-macro-13", cbuf13, 0} ,
	{"execute-macro-14", cbuf14, 0} ,
	{"execute-macro-15", cbuf15, 0} ,
	{"execute-macro-16", cbuf16, 0} ,
	{"execute-macro-17", cbuf17, 0} ,
	{"execute-macro-18", cbuf18, 0} ,
	{"execute-macro-19", cbuf19, 0} ,
	{"execute-macro-20", cbuf20, 0} ,
	{"execute-macro-21", cbuf21, 0} ,
	{"execute-macro-22", cbuf22, 0} ,
	{"execute-macro-23", cbuf23, 0} ,
	{"execute-macro-24", cbuf24, 0} ,
	{"execute-macro-25", cbuf25, 0} ,
	{"execute-macro-26", cbuf26, 0} ,
	{"execute-macro-27", cbuf27, 0} ,
	{"execute-macro-28", cbuf28, 0} ,
	{"execute-macro-29", cbuf29, 0} ,
	{"execute-macro-30", cbuf30, 0} ,
	{"execute-macro-31", cbuf31, 0} ,
	{"execute-macro-32", cbuf32, 0} ,
	{"execute-macro-33", cbuf33, 0} ,
	{"execute-macro-34", cbuf34, 0} ,
	{"execute-macro-35", cbuf35, 0} ,
	{"execute-macro-36", cbuf36, 0} ,
	{"execute-macro-37", cbuf37, 0} ,
	{"execute-macro-38", cbuf38, 0} ,
	{"execute-macro-39", cbuf39, 0} ,
	{"execute-macro-40", cbuf40, 0} ,
	{"execute-named-command", namedcmd, 0} ,
#if	PROC
	{"execute-procedure", execproc, 0} ,
#endif
	{"execute-program", execprg, 0} ,
	{"exit-emacs", quit, 0} ,
#if	WORDPRO
	{"fill-paragraph", fillpara, 1} ,
#endif
	{"filter-buffer", filter_buffer, 1} ,
	{"find-file", filefind, 0} ,
	{"forward-character", (fnp_t) forwchar, 0} ,
	{"goto-line", gotoline, 0} ,
#if	CFENCE
	{"goto-matching-fence", getfence, 0} ,
#endif
	{"grow-window", enlargewind, 0} ,
	{"handle-tab", insert_tab, 0} ,
	{"hunt-forward", forwhunt, 0} ,
	{"hunt-backward", backhunt, 0} ,
	{"help", help, 0} ,
	{"i-shell", spawncli, 0} ,
#if	ISRCH
	{"incremental-search", fisearch, 0} ,
#endif
	{"insert-file", insfile, 1} ,
	{"insert-space", insspace, 1} ,
	{"insert-string", istring, 1} ,
#if	WORDPRO
#if	PKCODE
	{"justify-paragraph", justpara, 1} ,
#endif
	{"kill-paragraph", killpara, 1} ,
#endif
	{"kill-region", killregion, 1} ,
	{"kill-to-end-of-line", killtext, 1} ,
	{"list-buffers", listbuffers, 0} ,
	{"meta-prefix", metafn, 0} ,
	{"move-window-down", mvdnwind, 0} ,
	{"move-window-up", mvupwind, 0} ,
	{"name-buffer", namebuffer, 0} ,
	{"newline", insert_newline, 1} ,
	{"newline-and-indent", indent, 1} ,
	{"next-buffer", nextbuffer, 0} ,
	{"next-line", (fnp_t) forwline, 0} ,
	{"next-page", (fnp_t) forwpage, 0} ,
#if	WORDPRO
	{"next-paragraph", gotoeop, 0} ,
#endif
	{"next-window", nextwind, 0} ,
	{"next-word", forwword, 0} ,
	{"nop", nullproc, 0} ,
	{"open-line", openline, 1} ,
	{"overwrite-string", ovstring, 0} ,
	{"pipe-command", pipecmd, 0} ,
	{"previous-line", (fnp_t) backline, 0} ,
	{"previous-page", (fnp_t) backpage, 0} ,
#if	WORDPRO
	{"previous-paragraph", gotobop, 0} ,
#endif
	{"previous-window", prevwind, 0} ,
	{"previous-word", backword, 0} ,
	{"query-replace-string", qreplace, 1} ,
	{"quick-exit", quickexit, 0} ,
	{"quote-character", quote, 1} ,
	{"read-file", fileread, 1} ,
	{"redraw-display", reposition, 0} ,
	{"resize-window", resize, 0} ,
	{"restore-window", restwnd, 0} ,
	{"replace-string", sreplace, 1} ,
#if	ISRCH
	{"reverse-incremental-search", risearch, 0} ,
#endif
#if	PROC
	{"run", execproc, 0} ,
#endif
	{"save-file", filesave, 1} ,
	{"save-window", savewnd, 0} ,
	{"scroll-next-up", scrnextup, 0} ,
	{"scroll-next-down", scrnextdw, 0} ,
	{"search-forward", forwsearch, 0} ,
	{"search-reverse", backsearch, 0} ,
	{"select-buffer", usebuffer, 0} ,
	{"set", setvar, 0} ,
	{"set-fill-column", setfillcol, 0} ,
	{"set-mark", (fnp_t) setmark, 0} ,
	{"shell-command", spawn, 0} ,
	{"shrink-window", shrinkwind, 0} ,
	{"split-current-window", splitwind, 0} ,
	{"store-macro", storemac, 0} ,
#if	PROC
	{"store-procedure", storeproc, 0} ,
#endif
#if	BSD | SVR4
	{"suspend-emacs", bktoshell, 0} ,
#endif
	{"transpose-characters", (fnp_t) twiddle, 1} ,
#if	AEDIT
	{"trim-line", trim, 1} ,
#endif
	{"unbind-key", unbindkey, 0} ,
	{"universal-argument", unarg, 0} ,
	{"unmark-buffer", unmark, 0} ,
	{"update-screen", upscreen, 0} ,
	{"view-file", viewfile, 0} ,
	{"wrap-word", wrapword, 0} ,
	{"write-file", filewrite, 0} ,
	{"write-message", writemsg, 0} ,
	{"yank", yank, 1} ,

	{"", NULL, 0}
};

/* end of names.c */
