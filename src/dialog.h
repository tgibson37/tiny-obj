#ifndef DIALOGHDR
#define DIALOGHDR

/************ data ************/
/************ functions ************/
int countch(char *f, char *t, char c);
/*	Returns the count of character c from f to t inclusive PLUS ONE.
	Used only to count lines starting with line 1. */
void errToWords();
/*	For each defined error prints a message with more info */
char* fchar(char* k);
/*	returns pointer to first character of the current line */
char* lchar(char* k);
/*	returns pointer to last character of the current line */
void whatHappened();
/*	Prints end of program message, "done" if no error, else code and 
 *	line with error and carot under. */
void showLine(char *line);
/*	Prints the line with the character */
/************ simple prints ******************/
void ps(char* s);
void pl(char* s);
int  pn(int n);
void pc(char c);
/*	same as tiny-c's ps,pn,pl,ps */
void logo();
/*	prints the logo remarks on startup. */
#endif
