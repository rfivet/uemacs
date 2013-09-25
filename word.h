#ifndef _WORD_H_
#define _WORD_H_

#define WORDPRO 1

int wrapword( int f, int n) ;
int backword( int f, int n) ;
int forwword( int f, int n) ;
int upperword( int f, int n) ;
int lowerword( int f, int n) ;
int capword( int f, int n) ;
int delfword( int f, int n) ;
int delbword( int f, int n) ;
#if WORDPRO
int gotobop( int f, int n) ;
int gotoeop( int f, int n) ;
int fillpara( int f, int n) ;
int justpara( int f, int n) ;
int killpara( int f, int n) ;
int wordcount( int f, int n) ;
#endif

#endif
