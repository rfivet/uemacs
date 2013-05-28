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

/* basic.c */
#include "basic.h"

/* display.c */
#include "display.h"

/* posix.c */
#include "termio.h"

/* input.c */
#include "input.h"

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
