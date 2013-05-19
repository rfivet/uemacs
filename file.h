#ifndef _FILE_H_
#define _FILE_H_

int fileread( int f, int n) ;
int insfile( int f, int n) ;
int filefind( int f, int n) ;
int viewfile( int f, int n) ;
int getfile( char *fname, int lockfl) ;
int readin( char *fname, int lockfl) ;
void makename( char *bname, char *fname) ;
void unqname( char *name) ;
int filewrite( int f, int n) ;
int filesave( int f, int n) ;
int writeout( char *fn) ;
int filename( int f, int n) ;
int ifile( char *fname) ;

#endif
