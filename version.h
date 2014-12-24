#ifndef VERSION_H_
#define VERSION_H_

#ifdef PROGRAM
# define _QUOTE( s) #s
# define QUOTE( s) _QUOTE( s)
# define PROGRAM_NAME QUOTE(PROGRAM)
#else
# define PROGRAM_NAME "em"
#endif

#define PROGRAM_NAME_LONG "uEMACS"

#define VERSION "4.1.1"

#endif  /* VERSION_H_ */
