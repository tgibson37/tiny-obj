#ifndef STACKHDR
#define STACKHDR
//#include "common.h"
#include "var.h"

/************ data ************/
/*	All tc data are ints or chars. Pointers are an int index into pr. 
 *	stuff is the typeless holder of any value.
 */
/* a stack entry */
struct stackentry { 
	int class; int lvalue; Type type; 
	union stuff value; 
};
/* the stack */
struct stackentry *stack;
int nxtstack, stacklen;

/************ functions ************/
void pushst( int class, int lvalue, Type type, union stuff *value );
/* basic pusher */
struct stackentry* popst();
/* basic popper, entry stays accessible until overwritten by push */
DATINT topdiff();
/*	resolve top two to actuals, pop them, return their difference */
DATINT toptoi();
/* pop the stack returning its int value, pointer 
	resolved and cast to int if necessary. */
void pushk(DATINT datum);
/* push an int */
void pushPtr(DATINT datum);
/* push an int as a class 1 */
void pushone();
/*	push an actual 1 */
void pushzero();
/*	push an actual 0 */
void dumpStackEntry(int e);
void dumpStack();
void dumpStackTo(int to);
void dumpPopTop();
void dumpTop();

#endif    /* STACKHDR */