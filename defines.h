/* Must define one of
	VMS | V7 | USG | BSD | MSDOS
*/
#define USG 1

#define PKCODE 1
#define SCROLLCODE 1   /* scrolling code P.K.                          */
#define ENVFUNC 1

#define	NSTRING	128		/* # of bytes, string buffers	*/

#define CONTROL 0x10000000	/* Control flag, or'ed in       */
#define META    0x20000000	/* Meta flag, or'ed in          */
#define CTLX    0x40000000	/* ^X flag, or'ed in            */
#define	SPEC	0x80000000	/* special key (function keys)  */

/* Actual 380x134 on a 1920x1080 screen in landscape,
   if smaller font or portrait orientation limit to 400x150 */
#define MAXCOL	400
#define MAXROW	150
