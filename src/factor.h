#ifndef FACTORHDR
#define FACTORHDR
#include "var.h"

void factor();
/*	Bottom of the expression stack (asgn(),reln(),...,factor).
	Recognizes constants, compound statements, syms, etc, and
	calls for appropriate treatment. */
struct var* obsym(char* qual);
void _pushvar(struct var *v);
char* mustFind( char *from, char *upto, char c, int err );
/*	Same as find except sets an err on no match */ 
#endif