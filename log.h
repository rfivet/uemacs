int rdonly( void) ;
int resterr( void) ;

extern void (*logwrite)( const char *, ...) ;
extern int (*logger)( int, int, const char *, ...) ;

