#ifndef VERSION_H_
#define VERSION_H_

#ifdef PROGRAM
# define _QUOTE( s) #s
# define QUOTE( s) _QUOTE( s)
# define PROGRAM_NAME QUOTE(PROGRAM)
#else
# define PROGRAM_NAME "em"
#endif

#define PROGRAM_NAME_UTF8 "µEMACS"

#define VERSION "4.2.4"

#endif  /* VERSION_H_ */
