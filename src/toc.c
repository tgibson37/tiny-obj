#include "toc.h"
#include "expr.h"
#include "stack.h"
#include "var.h"
#include "platform.h"
#include "factor.h"

struct varhdr *__temp_vh__;

#if defined(_WIN32)
        char* defaultLibrary = "pps\\library.tc";
#else
        char* defaultLibrary = "pps/library.tc";
#endif

/*	Modified tc.c
 */

 /************** literals **************/

/* stored size of one datum */
int typeToSize( int class, Type type ) {
	if(class)return sizeof(void*);
//	if(type=='A')return 0;
//	if(type=='C')return 0;
	if(type=='o')return sizeof(void*);
	if(type==Char)return 1;
	else if(type==Int)return sizeof(DATINT);
	else eset(TYPEERR);
	return 0; /* has to be one of the above */
}

/******* set error unless already set, capture cursor in errat *******/
void eset( int err ){
	if(!error){
		fflush(stdout);
		error=err;
		errat=cursor;
	}
}

/* Bump cursor over whitespace. Then return true on match and advance
   cursor beyond the literal else false and do not advance cursor
 */
int lit(char *s){
	while( *cursor == ' ' 
		|| *cursor == '\t' ) ++cursor;
	int match = !strncmp(s,cursor, strlen(s));
	if( match ) {
		cursor += strlen(s);
		return 1;
	}
	return 0; /* no match */
}

/* skips balance l-r assuming left is matched. 
 *	Returns # chars examined (delta) on OK, else -1.
 *  Does not change state, just examines.
 */
int skip_tool(char l, char r, char* from, char* to){
  char *bf = from;
  int counter = 1;
  while( counter>0 && from<endapp ) {
    if(*from==l)++counter;
    else if(*from==r)--counter;
    ++from;
  };
  int delta = from-bf;
  if( counter )return -1;   //bad
  return delta;
}

/*  convenience skip for parsing. Returns boolean 0/1.
 *	True means problem, 0 means ok and cursor advanced. 
 *	That is the old 8080 definition, returned CY bit unset/set.
 */
int skip(char l, char r) {
  int s = skip_tool(l,r,cursor,endapp);
  if(s<0)return 1;   //bad
  cursor += s;
  return 0;          //good
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
	else return 0;							/* not a symbol */
//	if(_isClassName(MAYBEDOT)) return 0;	/* ditto */
	while( isalnum(*temp) || *temp=='_'){
		++temp; 
	}
	lname = temp-1;
	if(verbose[VP]){
		fprintf(stderr,"\nparsed ");
		dumpft(fname,lname);
	}
	return lname-fname+1;  /* good, fname and lname defined */
}

/****************** some C helper functions **************/

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
char* _mustFind( char *from, char *upto, char c, int err ) {
	char* x = find(from, upto, c);
	if( x ) return x;
	else { eset(err); return 0; }
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

/* 	SITUATION: int, char or class-name-no-dot is parsed.
 *	Parses one variable. Makes allocation and symbol entry.
 */
void varalloc(Type type, struct var *varparent
		, union stuff *vpassed, struct varhdr *vh) {
	if(!symName()){eset(SYMERR);return;}
	cursor=lname+1;
	if( lit("(") ){
		vclass = 1;		/* array or pointer */
		char* fn=fname; /* localize globals that _asgn() may change */
		char* ln=lname;
		if( asgn() ) alen=toptoi()+1;  /* dimension */
		fname=fn; 		/* restore the globals */
		lname=ln;
		char* x = _mustFind(cursor,cursor+5,')',RPARERR);
		if(x)cursor = x+1;
	} else {
		vclass = 0;
		alen = 1;
	}
	newvar(vclass, type, alen, varparent, vpassed, vh);
}

/********** expression parser *****************/

/************ scan tools ******************/

/*	skip( a possibly compound statement. Shortcoming is brackets
 *	in comments, they must be balanced.
 */
void _skipSt() {
	rem();
	if( lit(xlb) ) {		/* compound */
		skip('[',']');
		rem();
		return;
	}
	else if( lit(xif)||lit(xwhile) ) {
		lit(xlpar);			/* optional left paren */
		skip('(',')');
		_skipSt();
		rem();
		if(lit(xelse))_skipSt();
		rem();
		return;
	}
	else {					/* simple statement, eol or semi ends */
		while(++cursor<endapp) {
			if( (*cursor==0x0d)||(*cursor=='\n')||(*cursor==';') )break;
		}
		++cursor;
		rem();
	}
}

/* Returns true if user signals quit, or any other error.
 *	NOTE: MC 2 esets KILL on ESC, but test here for hard loop
 */

#if defined(_WIN32)
int quit() {    // mod's lrb
 int x;
 if (kbhit()) {
  x = getch_(ECHO);
  if (x == ESCAPE) {
   eset(KILL);
   return 1;
   }
  }
 return 0;
}
#else
int quit() {
//	int foo[]={0};  /* to avoid tcc error */
	if(kbhit()==0x1b){
		getch_(0);
		return escKey();
	}
	return 0; 
}
#endif

/* Match char, int, or class, else do nothing. If match parse
 *  all comma separated declarations of that particular type
 *	making var table entries and allocating value storage. Returns false
 *	if not a declaration statement, true if it is. Leaves cursor just past
 *  optional semi. 
 */
int decl(struct varhdr *vh) {
//	struct var *isvar;
	if( lit(xchar) ) {
		do {
			varalloc( Char, NULL, NULL, vh );
		} while( lit(xcomma) );
	} 
	else if( lit(xint) ) {
		do {
			varalloc( Int, NULL, NULL, vh );
		} while( lit(xcomma) );
	}
#if 0
	else if((isvar=_isClassName(NODOT))) {
		if(symName()) {   // decl of var of type 'o'
			do {
				varalloc( 'o', isvar, NULL, vh );
			} while( lit(xcomma) );
		}
	}
#endif
	else {
		return 0;  /* not decl */
	}
	lit(xsemi);    /* is decl */
	return 1;
}

/* st(): interprets a possibly compound statement */
char *prevcur = NULL;
char *appcur = NULL;
void st() {
	char *objt, *agin ;
	struct var *isvar;
	brake=0;
	if(cursor==prevcur){
		eset(FREEZERR);
		whatHappened();
		exit(FREEZERR);
	}
	prevcur=cursor;
	if(cursor>apr)appcur=cursor;
	if(quit())return;
	rem();
	stbegin();
	stcurs = cursor;
	if(decl(locals)){
		rem();
		return;
	}
	else if( lit(xlb) ){     /* compound statement */
		for(;;){
			rem();
			if(leave||brake||error)return;
			if(lit(xrb)){
				rem();
				return;
			}
			st();
		}
	}
	else if(lit(xif)) {
		if(asgn()) {
			if(toptoi()) {
				st();
				rem();
				if(lit(xelse)) {
					_skipSt();
				}
			} 
			else {
				_skipSt();
				rem();
				if(lit(xelse)) {
					st();
				}
			}
			rem();
			return;
		}
	}
	else if(lit(xwhile)) {
		lit(xlpar);    /* optional left paren */
		if( !asgn() )return;   /* error */
		lit(xrpar);
		int condition = toptoi();
		if( condition ) {
/* prepare for repeating/skipping while (stcurs) 
	or object */ 
			agin = stcurs;
			objt = cursor;
			st();

			if(brake) {
				cursor = objt;	/* break: done with the while */
				_skipSt();		/* skip over the object */
				brake = 0;
				return;
			}
			else {
				cursor = agin;	/* no break: do the entire while again */
				return;
			}
		}
		else {
			_skipSt();
		}
	}
	else if(lit(xsemi)) {
		rem();
	}
	else if(lit(xreturn)) {
		int eos = ( lit(xrpar)
					 || *cursor==*xlb
					 || *cursor==*xrb
					 || *cursor==*xsemi
					 || *cursor==*xnl
					 || *cursor==0x0d
					 || *cursor==*xslash
					);
		if ( eos ) {
			pushzero(); /* default return value */
		}
		else {
			asgn();  /* specified return value */
		}
		leave=1;		/* signal st() to leave the compound 
						statement containing this return */
		return;
	}
	else if(lit(xbreak)) {
		brake=1;
		return;
	}
#if 1
	else if((isvar=_isClassName(NODOT))) {
		if(symName()) {   // decl of var of type 'o'
			do {
				varalloc( 'o', isvar, NULL, locals );
			} while( lit(xcomma) );
		}
	}
#endif
	else if(lit(xdelete)){
		if(symName()){
			struct var sym;
			canon(&sym);
			cursor = lname+1;
			struct var *v = addrval_all(sym.name);
			if(!v){eset(SYMERR); return;}
			void *vh = *v->vdcd.od.blob;
			if(vh)free(vh);
			else eset(SYNXERR);
		}
		else eset(SYNXERR);
	}
	else if( asgn() ) {      /* if expression discard its value */
		popst();
        lit(xsemi);
	}
	else {
		eset(STATERR);
	}
}

/*********** a variety of dumps for debugging **********/
void dumpHex( void* where, int len ) {
	void* w=where;
	fflush(stdout);
	fprintf(stderr,"\n%p: ",where);
	int i;
	for( i=0; i<len; ++i )fprintf(stderr," %p",w+i);
}

/* dump from..to inclusive  */
void dumpft(char *from, char *to ) {
	fflush(stdout);
	while(from <= to) fprintf(stderr,"%c",*(from++));
}
void dumpSym(){
	if(fname&&lname)dumpft(fname,lname);
	else fprintf(stderr,"-NO Sym-");
}
/* dump the line cursor is in from cursor to nl */
void dumpLine() {
	fflush(stdout);
	char* begin = cursor;
	char* end = cursor;
	while (*end!=0x0a && *end!=0x0d && end<endapp){  /* find end of line */
		++end;
	}
	if(begin==end)fprintf(stderr,"dumpLine: cursor at end of line");
	while(begin<end){
		fprintf(stderr,"%c",*begin);
		++begin;
	}
}

int stateCallNumber=0;
void dumpState() {
	fflush(stdout);
	if(stateCallNumber==0){
		fprintf(stderr,"\nADDRESSES (hex)");
		fprintf(stderr,"\npr:     %p",(void*)pr);
		fprintf(stderr,"\nstack:  %p",(void*)stack);
//		fprintf(stderr,"\nvartab: %p",(void*)vartab);	
	}

	fprintf(stderr,"\n----\nSTATE %d\nparsing: ",stateCallNumber++);
	dumpLine();
	fprintf(stderr,"<--end of line--");
//	dumpVarTab();
	dumpStack();
}

/* dump a just parsed piece of pr, typically a name */
void dumpName() {
	fflush(stdout);
	fprintf(stderr,"\njust parsed: ");
	char *c = fname;
	while( c <= lname ) { 
			fprintf(stderr,"%c",*(c++));
	}
	fflush(stdout);
}

/****  C tools to deal with typeless storage ****/
void put_int(char *where, DATINT datum) {
	memcpy( where, &datum, sizeof(datum) );
}
DATINT get_int(char *where) {
	int datum;
	memcpy( &datum, where, sizeof(datum));
	return datum;
}
void put_char(char *where, char datum) {
	memcpy(where, &datum, sizeof(datum));
}
char get_char(char *where) {
	char datum;
	memcpy( &datum, where, sizeof(datum));
	return datum;
}
