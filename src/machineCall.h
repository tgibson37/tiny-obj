#ifndef MACHINECALLHDR
#define MACHINECALLHDR

#include "stack.h"
#include "var.h"
#include "fileRead.h"
#include "toc.h"
#include <math.h>

/*************** data ***************/
/************* functions ************/
/*	The topmost nargs stack entries are args to this machine call. 
	The MC number is the last parsed, hence on the stack top. */
void machinecall( int nargs );

#endif    /* MACHIINECALLHDR */