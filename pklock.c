/* pklock.c -- implements pklock.h */
#include "pklock.h"

/*	PKLOCK.C
 *
 *	locking routines as modified by Petri Kutvonen
 */

#if (FILOCK && BSD) || SVR4
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "util.h"

/* Maximum file length name 255 */
#define MAXLOCK 256
#define MAXNAME 128

#if defined(SVR4) && ! defined(__linux__)
#include <sys/systeminfo.h>

int gethostname(char *name, int namelen)
{
	return sysinfo(SI_HOSTNAME, name, namelen);
}
#endif

char *cuserid( char *retbuf) ;	/* should have been declared in stdio.h */


/**********************
 *
 * if successful, returns NULL  
 * if file locked, returns username of person locking the file
 * if other error, returns "LOCK ERROR: explanation"
 *
 *********************/
char *dolock( const char *fname)
{
	int fd, n;
	char lname[ MAXLOCK] ;
	static char locker[ MAXNAME + 1] ;
	int mask;
	struct stat sbuf;

	mystrscpy( lname, fname, sizeof lname - 6) ;
	strcat( lname, ".lock~") ;

	/* check that we are not being cheated, qname must point to     */
	/* a regular file - even this code leaves a small window of     */
	/* vulnerability but it is rather hard to exploit it            */

#if defined(S_IFLNK)
	if (lstat(lname, &sbuf) == 0)
#else
	if (stat(lname, &sbuf) == 0)
#endif
#if defined(S_ISREG)
		if (!S_ISREG(sbuf.st_mode))
#else
		if (!(((sbuf.st_mode) & 070000) == 0))	/* SysV R2 */
#endif
			return "LOCK ERROR: not a regular file";

	mask = umask(0);
	fd = open(lname, O_RDWR | O_CREAT, 0666);
	umask(mask);
	if (fd < 0) {
		if (errno == EACCES)
			return NULL;
#ifdef EROFS
		if (errno == EROFS)
			return NULL;
#endif
		return "LOCK ERROR: cannot access lock file";
	}
	if ((n = read(fd, locker, MAXNAME)) < 1) {
		lseek(fd, 0, SEEK_SET);
/*
**	Since Ubuntu 17.04, cuserid prototype seems missing. Replacing it by
**  getlogin does the trick on 64 bits but fails on 32 bits.
**  So let's work around with cuserid for a while.
**		logname = getlogin() ;
**		strcpy( locker, logname ? logname : cuserid( NULL)) ;
*/
#if BSD
		strcpy( locker, getlogin()) ;
#else
		strcpy( locker, cuserid( NULL)) ;
#endif

		strcat(locker + strlen(locker), "@");
		gethostname(locker + strlen(locker), 64);
		{
			int ret, locker_size ;

			locker_size = strlen( locker) ;
			ret = write( fd, locker, locker_size) ;
			if( ret != locker_size) {
			/* some error handling here */
			}
		}

		close(fd);
		return NULL;
	}
	locker[n > MAXNAME ? MAXNAME : n] = 0;
	return locker;
}


/*********************
 *
 * undolock -- unlock the file fname
 *
 * if successful, returns NULL
 * if other error, returns "LOCK ERROR: explanation"
 *
 *********************/

char *undolock( const char *fname) {
	char lname[ MAXLOCK] ;

	mystrscpy( lname, fname, sizeof lname - 6) ;
	strcat( lname, ".lock~") ;
	if (unlink(lname) != 0) {
		if (errno == EACCES || errno == ENOENT)
			return NULL;
#ifdef EROFS
		if (errno == EROFS)
			return NULL;
#endif
		return "LOCK ERROR: cannot remove lock file";
	}
	return NULL;
}

#else
typedef void _pedantic_empty_translation_unit ;
#endif

/* end of pklock.c */
