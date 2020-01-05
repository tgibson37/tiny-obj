#include "stack.h"
<<<<<<< HEAD
#include "tc.h"
=======
#include "toc.h"

/*	prints a value given its description taken from a struct stackEntry
 */
void dumpVal_s(Type t, int class, union stuff *val, char lval){
  if(class==1 && t==Char ){           // string
    char sval[30];
    strncpy(sval, (char*)(val->up), 30);
    fprintf(stderr,"->%s<-", sval);
  }
  else if(class==1 || lval=='L'){     // pointer
  	void *ptr=val->up;
//fprintf(stderr,"\n           stack~13, %d %d",pr<=(char*)ptr, (char*)ptr<EPR);
//fprintf(stderr,"\n           stack~14, %p %p %p",pr, (char*)ptr, EPR);
    if(pr<=(char*)ptr && (char*)ptr<EPR) {
      int p = (void*)pr-val->up;
      fprintf(stderr,"pr[%d]",p);
    }
    else fprintf(stderr,"ptr");
  }
  else if(t==Char) fprintf(stderr,"%c",val->uc);   // actual datum
#if defined(_WIN32)
  else fprintf(stderr,"%Id",val->ui);
#else
  else fprintf(stderr,"%td",val->ui);
#endif
}
>>>>>>> newbase

void dumpStackEntry(int e){
	fflush(stdout);
	if( 0<=e && e<=nxtstack ) {
		fprintf(stderr,"\n stack entry at %d: %d %c %d ", e, stack[e].class, 
			stack[e].lvalue, stack[e].type );
		if(verbose[VS])dumpVal(stack[e].type, stack[e].class, 
				&stack[e].value,stack[e].lvalue);
	}
	else {
		fprintf(stderr,"no stack entry at %d", e);
	}
}
void dumpStack(){
	int e;
	fflush(stdout);
	fprintf(stderr,"\nStack (from top) class lvalue type stuff");
	for( e=nxtstack-1; e>=0; --e) {
		dumpStackEntry(e); 
	}
	fprintf(stderr,"\n");
}
/* dumps the just popped stack entry */
void dumpPopTop() {
	dumpStackEntry(nxtstack);
}
/* dumps the top stack entry */
void dumpTop() {
	dumpStackEntry(nxtstack-1);
}

void stuffCopy( union stuff *to, union stuff *from ) {
	memcpy( to, from, sizeof(*to));
}

/* basic pusher */
void pushst( int class, int lvalue, Type type, union stuff *value ) {
	if( nxtstack > stacklen) { error = PUSHERR; return; }
	stack[nxtstack].class = class;
	stack[nxtstack].lvalue = lvalue;
	stack[nxtstack].type = type;
	stuffCopy( &stack[nxtstack].value, value);
	++nxtstack;
	if(verbose[VS]){
		fprintf(stderr,"\nstack push: ");
		dumpStackEntry(nxtstack-1);
	}
}

/* basic popper, entry stays accessible until pushed over */
struct stackentry* popst() {
	if( nxtstack-1 < 0 ) { error = POPERR; return NULL; }
	if(verbose[VS]){
		fprintf(stderr,"\nstack pop: ");
		dumpStackEntry(nxtstack-1);
	}
	--nxtstack;
	return &stack[nxtstack];
}

/************ derived convenient pushers and poppers ************/

DATINT topdiff() {
<<<<<<< HEAD
	DATINT b = toptoi();
	DATINT a = toptoi();
	return ( a-b );
}

/* pop the stack returning its integer value, pointer 
	resolved and cast if necessary. */
DATINT toptoi() {
	DATINT datum;
=======
	int b = toptoi();
	int a = toptoi();
	return ( a-b );
}

/* pop the stack returning its int value, pointer 
	resolved and cast to int if necessary. */
DATINT toptoi() {
	int datum;
>>>>>>> newbase
	union stuff *ptr;
	if(verbose[VS]){
		fprintf(stderr,"\ntoptoi pop: ");
		dumpStackEntry(nxtstack-1);
	}

	struct stackentry *top = &stack[--nxtstack];
	if( (*top).class==1 ) {
		if((*top).lvalue == 'L') {
			ptr = (union stuff *)((*top).value.up);
			datum=(DATINT)((*ptr).up);
		}
		else datum=(DATINT)((*top).value.up);
	}
	else if((*top).lvalue == 'L') {
		if((*top).type==Int ) datum = *((DATINT*)((*top).value.up));
		else if((*top).type==Char) datum = *((char*)((*top).value.up));
<<<<<<< HEAD
=======
		else if((*top).type == 'o') {
			datum = (DATINT)(*top).value.up;
		}
>>>>>>> newbase
		else eset(TYPEERR);
	}
	else if((*top).lvalue == 'A') {
		if((*top).type==Char) datum  = (char)((*top).value.uc);
		else if((*top).type==Int) datum  = ((*top).value.ui);
		else eset(TYPEERR);
	}
	else { eset(LVALERR); }
	if(verbose[VS]){
		fprintf(stderr," -- toptoi ");
// format FMTINT matches DATINT type (choice in common.h)
		fprintf(stderr,FMTINT, datum);
	}
	return datum;
}

/* push an int */
void pushk(DATINT datum) {
	union stuff d;
	d.ui = datum;
	pushst( 0, 'A', Int, &d );
//fprintf(stderr," -- pushk %d",datum);
}

/* push an int as a class 1 */
void pushPtr(DATINT datum) {
	union stuff d;
	d.up = (void*)datum;
	pushst( 1, 'A', Int, &d );
}

/* these two used by RELN */
void pushone() {
	pushk(1);
}
void pushzero() {
	pushk(0);
}
