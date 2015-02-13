/* mlout.h -- message line output interface */

#ifndef __MLOUT_H__
#define __MLOUT_H__

extern void (*mloutfmt)( const char *, ...) ;

void mloutstr( const char *str) ;

#endif /* __MLOUT_H__ */

/* end of mlout.h */
