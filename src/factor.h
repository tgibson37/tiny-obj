#ifndef FACTORHDR
#define FACTORHDR

void factor();
/*	Bottom of the expression stack (asgn(),reln(),...,factor).
	Recognizes constants, compound statements, syms, etc, and
	calls for appropriate treatment. */
char* mustFind( char *from, char *upto, char c, int err );
/*	Same as find except sets an err on no match */ 
#endif