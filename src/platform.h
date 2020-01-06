#ifndef PLATFORMHDR
#define PLATFORMHDR

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
/*************  Platform stuff ************/
/* INSTALLERS/PORTERS may need to redefine these functions.
	Current choices: Windows, else linux.
 */

#if defined(_WIN32)

// WINDOWS, uses its lib code except getch_
#include <conio.h>
char getch_(int echo);
#define ECHO 1

#else

//LINUX from here to end
// (works on mint linux, a derivitave of ubuntu)
#include <termios.h>
/*************** data ***************/
// kbhit needed this. Too delicate to remove yet.
//static struct termios old, new;
/************* functions ************/
int kbhit(void);
/*	Detect a key hit is waiting in its buffer, return the char,
 *	leaving it in the buffer to be read. Used to detect ESC character
 *	to force a quit. Return 0 if no character waiting.
 */
void initTermios(int echo);
/* Initialize new terminal i/o settings */
void resetTermios(void);
/* Restore old terminal i/o settings */
char getch_(int echo);
/* Non blocking read 1 character. echo defines echo mode. */
char getch(void);
/* Read 1 character without echo */
char getche(void);
/* Read 1 character with echo */

#endif    // Linux section

#endif    // PLATFORMHDR include guard
