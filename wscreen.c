/* wscreen.c -- windows screen console */
#include "wscreen.h"

#ifdef MINGW32

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

/* Standard error macro for reporting API errors */ 
#define PERR(bSuccess, api){if(!(bSuccess)) printf("%s:Error %d from %s \
    on line %d\n", __FILE__, GetLastError(), api, __LINE__);}

static void cls( HANDLE hConsole )
{
    COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                                        cursor */ 
    BOOL bSuccess;
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */ 
    DWORD dwConSize;                 /* number of character cells in
                                        the current buffer */ 

    /* get the number of character cells in the current buffer */

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
    PERR( bSuccess, "GetConsoleScreenBufferInfo" );
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    /* fill the entire screen with blanks */ 

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
       dwConSize, coordScreen, &cCharsWritten );
    PERR( bSuccess, "FillConsoleOutputCharacter" );

    /* get the current text attribute */ 

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
    PERR( bSuccess, "ConsoleScreenBufferInfo" );

    /* now set the buffer's attributes accordingly */ 

    bSuccess = FillConsoleOutputAttribute( hConsole, csbi.wAttributes,
       dwConSize, coordScreen, &cCharsWritten );
    PERR( bSuccess, "FillConsoleOutputAttribute" );

    /* put the cursor at (0, 0) */ 

    bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );
    PERR( bSuccess, "SetConsoleCursorPosition" );
    return;
}

void wcls( void) {
    cls( GetStdHandle( STD_OUTPUT_HANDLE)) ;
}
 
static struct {
	int width ;
	int height ;
	int curTop, curBot, curRight, curLeft ;
} Screen ;

void winit( void) {
    CONSOLE_SCREEN_BUFFER_INFO    csbInfo ;

    wcls() ;
    if( GetConsoleScreenBufferInfo(
                                 GetStdHandle( STD_OUTPUT_HANDLE), &csbInfo)) {
        Screen.width = csbInfo.dwSize.X ;
	Screen.height = csbInfo.dwSize.Y ;
        Screen.curLeft = csbInfo.srWindow.Left ;
	Screen.curTop = csbInfo.srWindow.Top ;
	Screen.curRight = csbInfo.srWindow.Right ;
	Screen.curBot = csbInfo.srWindow.Bottom ;
    }
}

int wwidth( void) {
	return Screen.width ;
}

int wheight( void) {
	return Screen.height ;
}

int wleft( void) {
	return Screen.curLeft ;
}

int wtop( void) {
	return Screen.curTop ;
}

int wright( void) {
	return Screen.curRight ;
}

int wbottom( void) {
	return Screen.curBot ;
}

void wgoxy( int x, int y) {
    COORD coord ;
    
    coord.X = x ;
    coord.Y = y ;
    SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE), coord );
}

void wtitle( const char *title) {
    SetConsoleTitle( title) ;
}

#endif

/* end of wscreen.c */
