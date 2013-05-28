#ifndef _EFUNC_H_
#define _EFUNC_H_

/*  efunc.h
 *
 *  Function declarations and names.
 *
 *  This file list all the C code functions used and the names to use
 *      to bind keys to them. To add functions, declare it here in both the
 *      extern function list and the name binding table.
 *
 *  modified by Petri Kutvonen
 */

/* External function declarations. */

/* word.c */
#include "word.h"

/* window.c */
#include "window.h"

/* basic.c */
#include "basic.h"

/* main.c */
#include "main.h"

/* display.c */
#include "display.h"

/* region.c */
#include "region.h"

/* posix.c */
#include "termio.h"

/* input.c */
#include "input.h"

/* bind.c */
#include "bind.h"

/* buffer.c */
#include "buffer.h"

/* eval.c */
#include "eval.h"

#if	BSD | SVR4
/* lock.c */
#include "lock.h"

#if (FILOCK && BSD) || SVR4
/* pklock.c */
#include "pklock.h"
#endif
#endif

#endif
