#include "toc.h"
#include "parse.h"

/*	parse tools 
 */
/* Bump cursor over whitespace. Then return true on match and advance
   cursor beyond the literal else false and do not advance cursor
 */
int lit(char *s){
	while( *cursor == ' ' || *cursor == '\t' ) ++cursor;
	int match = !strncmp(s,cursor, strlen(s));
	if( match ) {
		cursor += strlen(s);
		return 1;
	}
	return 0; /* no match */
}

/* skips balance l-r assuming left is matched. 
 *	Returns 0 on OK, else count of missing ]'s.
 */
int skip(char l, char r) {
	int counter = 1;
	 while( counter>0 && cursor<endapp ) {
		if(*cursor==l)++counter;
		if(*cursor==r)--counter;
		++cursor;
	};
	if( counter )return counter;
	return 0;
}

/* Parse a symbol defining fname, lname. ret: true if symbol.
 *	Advances the cursor to but not over the symbol,
 */
int symName() {
	char* temp;
	while( *cursor == ' ' || *cursor == '\t' ) ++cursor;
	temp=cursor;
	if( isalpha(*temp) || *temp=='_') {
		fname = temp;
	}
	else return 0;  /* not a symbol */
	while( isalnum(*temp) || *temp=='_'){
		++temp; 
	}
	lname = temp-1;
//	if(verbose[VP]){
//		fprintf(stderr,"\nparsed ");
//		dumpft(fname,lname);
//	}
	return lname-fname+1;  /* good, fname and lname defined */
}

/****************** some C helper functions **************/

/* return true if symname matches arg, no state change */
int symNameIs(char* name){
	int x = strncmp(fname, name, lname-fname+1);
	return( !x );
}
/* State is not changed by find or mustFind. Returned value is
sole purpose of find. That plus setting err for mustFind. */
char* find( char* from, char* upto, char c) {
	char* x = from;
	while( *x != c && x<upto) {
		++x;
	}
	return x<upto ? x : 0;
}
/* same as find but sets err on no match */
char* mustFind( char *from, char *upto, char c, int err ) {
	char* x = find(from, upto, c);
	if( x ) return x;
	else { eset(err); return 0; }
}

/* special find for end of string */
char* findEOS( char* x ) {
	while( x<endapp) {
		if( *x==0 || *x==0x22 ) return x;
		++x;
	}
	eset(CURSERR);
	return 0;
}

/* skip over comments and/or empty lines in any order, new version
	tolerates 0x0d's, and implements // as well as old slash-star comments.
 */
void rem() {
	for(;;) {
		while(    *cursor==0x0a
				||*cursor==0x0d
				||*cursor==' '
				||*cursor=='\t'
			  )++cursor;
		if( !(lit(xcmnt)||lit(xcmnt2)) ) return;
		while( *cursor != 0x0a && *cursor != 0x0d && cursor<endapp )
			++cursor;
	}
}

