/* isa.h -- isletter, islower, isupper, flipcase */
#ifndef __ISA_H__
#define __ISA_H__

#ifdef  islower
# undef islower
#endif

#ifdef  isupper
# undef isupper
#endif

#define NATIONL 0   /* if 1, interpret [,],\,{,},| as characters P.K.    */
#if NATIONL
# define LASTUL ']'
# define LASTLL '}'
#else
# define LASTUL 'Z'
# define LASTLL 'z'
#endif

#define isletter(c) __isxletter((0xFF & (c)))
#define islower(c)  isxlower((0xFF & (c)))
#define isupper(c)  isxupper((0xFF & (c)))

#define __isxletter(c)  (('a' <= c && LASTLL >= c) || \
                        ('A' <= c && LASTUL >= c) || (192<=c /* && c<=255 */))
#define isxlower(c) (('a' <= c && LASTLL >= c) || (224 <= c && 252 >= c))
#define isxupper(c) (('A' <= c && LASTUL >= c) || (192 <= c && 220 >= c))

/* DIFCASE represents the integer difference between upper and lower
   case letters. It is an xor-able value, which is fortunate, since the
   relative positions of upper to lower case letters is the opposite of
   ascii in ebcdic.
*/

#define DIFCASE 0x20                    /* ASCII 'a' - 'A' */
#define flipcase( c) ((c) ^ DIFCASE)    /* Toggle the case of a letter. */

#endif
/* end of isa.h */
