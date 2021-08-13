/* version.h -- name and version strings */
#ifndef _VERSION_H_
#define _VERSION_H_

#ifdef PROGRAM
# define _QUOTE( s) #s
# define QUOTE( s) _QUOTE( s)
# define PROGRAM_NAME QUOTE(PROGRAM)
#else
# define PROGRAM_NAME "ue"
#endif

#define PROGRAM_NAME_UTF8 "µEMACS"

#define VERSION "4.2.5"

#endif
/* end of version.h */
