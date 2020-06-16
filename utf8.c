/* utf8.c -- implements utf8.h, converts between unicode and UTF-8 */

#define _XOPEN_SOURCE	/* wcwidth in wchar.h */

#include "utf8.h"

#include <assert.h>
#include <wchar.h>

/*
 * Display width of UTF-8 character
 */
unsigned utf8_width( unicode_t c) {
#if CYGWIN
	assert( sizeof( wchar_t) == 2) ;	/* wcwidth only handles UTF-16 */
	return (c < 0x10000) ? (unsigned) wcwidth( (wchar_t) c) : 2 ;
#elif BSD
	assert( sizeof( wchar_t) == 4) ;	/* wcwidth should handle UTF-32 */
	return 1 ;
#else
	return (unsigned) wcwidth( (wchar_t) c) ;
#endif
}

/*
 * utf8_to_unicode()
 *
 * Convert a UTF-8 sequence to its unicode value, and return the length of
 * the sequence in bytes.
 *
 * NOTE! Invalid UTF-8 will be converted to a one-byte sequence, so you can
 * either use it as-is (ie as Latin1) or you can check for invalid UTF-8
 * by checking for a length of 1 and a result > 127.
 *
 * NOTE 2! This does *not* verify things like minimality. So overlong forms
 * are happily accepted and decoded, as are the various "invalid values".
 */
unsigned utf8_to_unicode( const char *line, unsigned index, unsigned len,
															unicode_t *res) {
    unicode_t   value ;
    unsigned	c ;
    unsigned	bytes, mask, i;

	if( index >= len)
		return 0 ;

    *res = c = line[ index] & 0xFFU ;

    /*
     * 0xxxxxxx is valid one byte utf8
     * 10xxxxxx is invalid UTF-8 start byte, we assume it is Latin1
     * 1100000x is start of overlong encoding sequence
     * Sequence longer than 4 bytes are invalid
     * Last valid code is 0x10FFFF, encoding start with 0xF4
     */
    if( c <= 0xC1 || c > 0xF4)
        return 1;

    /* Ok, it's 11xxxxxx, do a stupid decode */
    mask = 0x20;
    bytes = 2;
    while( (c & mask) != 0) {
        bytes++;
        mask >>= 1;
    }

	/* bytes is in range [2..4] as c was in range [C2..F4] */
    len -= index;
    if (bytes > len)
        return 1;

    value = c & (mask-1);

    /* Ok, do the bytes */
    line += index;
    for (i = 1; i < bytes; i++) {
        c = line[i] & 0xFFU ;
        if ((c & 0xc0) != 0x80)
            return 1;
        value = (value << 6) | (c & 0x3f);
    }

    if( value > 0x10FFFF) /* Avoid 110000 - 13FFFF */
        return 1 ;

    *res = value;
    return bytes;
}

/*
 * unicode_to_utf8()
 *
 * Convert a unicode value to its canonical utf-8 sequence.
 *
 * NOTE! This does not check for - or care about - the "invalid" unicode
 * values.  Also, converting a utf-8 sequence to unicode and back does
 * *not* guarantee the same sequence, since this generates the shortest
 * possible sequence, while utf8_to_unicode() accepts both Latin1 and
 * overlong utf-8 sequences.
 */
unsigned unicode_to_utf8( unicode_t c, char *utf8) {
    unsigned bytes = 1 ;

    assert( c <= 0x10FFFF) ;

#ifdef NDEBUG
	if( c > 0x10FFFF)	/* Let's assume this is due to sign extension */
		c &= 0xFF ;
#endif

    if( c <= 0x7f)
	    *utf8 = (char) c ;
    else {
        unsigned prefix = 0x40 ;
        char *p = utf8 ;
        do {
            *p++ = (char) (0x80 + (c & 0x3f)) ;
            bytes++ ;
            prefix >>= 1 ;
            c >>= 6 ;
        } while( c >= prefix) ;

		*p-- = *utf8 ;
		*utf8++ = (char) (c - 2 * prefix) ;
		if( utf8 < p) {	/* swap middle two bytes if 4 bytes utf-8 code */
			char c = *p ;
			*p = *utf8 ;
			*utf8 = c ;
		}
    }

    return bytes ;
}

unsigned utf8_revdelta( unsigned char *p, unsigned pos) {
	unsigned delta = 0 ;

	if( (*p & 0xC0) == 0x80) {
		unsigned char c ;

		c = *--p ;
		if( (c & 0xE0) == 0xC0)	/* valid 2 bytes unicode seq */
			delta = 1 ;
		else if( ((c & 0xC0) == 0x80) && (pos > 1)) {
			c = *--p ;
			if( (c & 0xF0) == 0xE0)	/* valid 3 bytes unicode seq */
				delta = 2 ;
			else if( ((c & 0xC0) == 0x80) && (pos > 2))
				if( (p[ -1] & 0xF8) == 0xF0)	/* valid 4 bytes unicode seq */
					delta = 3 ;
		}
	}

	return delta ;
}


/* end of utf8.c */
