/* utf8.h -- */
#ifndef _UTF8_H_
#define _UTF8_H_

typedef unsigned int unicode_t ;

int _utf8_width( unicode_t c) ;	/* straight width */
int  utf8_width( unicode_t c) ;	/* workaround width */
unsigned utf8_to_unicode( const char *line, unsigned index, unsigned len,
                                                            unicode_t *res) ;
unsigned utf8_revdelta( unsigned char *buf, unsigned pos) ;
unsigned unicode_to_utf8( unicode_t c, char *utf8) ;

#endif
/* end of utf8.h */
