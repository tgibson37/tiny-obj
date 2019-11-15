#include "toc.h"

int varargs = 0;
union stuff foo;

void pushvar(struct var *v){
		  	int class=v->vdcd.vd.class; 
	  		int type=v->type; 
	  		int obsize = typeToSize(class,type);
		  	char* where = v->vdcd.vd.value.up;
	  		int len=v->vdcd.vd.len;
				if( _lit(xlpar) ) {		       /* is dimensioned */
			  		if( !class ) {   /* must be class>0 */
						eset(CLASERR);
			  		} else {  /* dereference the lvalue */
			  			/* reduce the class and recompute obsize */
	  					obsize = typeToSize(--class,type);
			  			/* increment where by subscript*obsize */
		        		_asgn(); if( error )return;
		        		_lit(xrpar);
			      		int subscript = toptoi();
						if(len-1)if( subscript<0 || subscript>=len )eset(RANGERR); 
						where += subscript * obsize;
						foo.up = where;
						pushst( class, 'L', type, &foo);
						return;
					}
				} else {	
				/* is simple. Must push as 'L', &storagePlace. */
					if(class==1){
						foo.up = &((*v).vdcd.vd.value.up);
					}
					else{
						foo.up = where;
					}
			  		pushst( class, 'L', type, &foo);
				}
}

/* return true if symname matches arg, no state change */
int symNameIs(char* name){
	int x = strncmp(fname, name, lname-fname+1);
	return( !x );
}

/* converts fname..lname inclusive to integer
 */
int _atoi() {
	char* x = fname;
	int val = 0;
	int sign = *x=='-' ? -1 : 1;
	if( *x=='-' || *x=='+' ) ++x;
	do{ 
		val = 10*val+*x-'0'; 
	} while( ++x <= lname );
	return sign*val;
}

/* special find for end of string */
char* _findEOS( char* x ) {
	while( x<endapp) {
		if( *x==0 || *x==0x22 ) return x;
		++x;
	}
	eset(CURSERR);
	return 0;
}

/* Situation: parsing argument declarations, passed values are on the stack.
 * arg points into stack to an argument of type. 
 * Gets actual value of arg, calls valloc which parses and sets
 * up local with the passed value.
 */ 
void _setArg( Type type, struct stackentry *arg, struct varhdr *locals ) {
	union stuff vpassed  = (*arg).value;
	char* where;
	int class = (*arg).class;
	int lvalue = (*arg).lvalue;
	int stacktype = (*arg).type;
	if( lvalue=='L') {
		where = (char*)vpassed.up;
		if( class==1 ) { 
			vpassed.up = *((char**)(*arg).value.up);
		}
		else if( stacktype==Int ) vpassed.ui = get_int(where);
		else if( stacktype==Char) vpassed.ui = get_char(where);
			/* ui to clear high order byte */
	}
	varalloc( type, &vpassed, locals);
}

/*	SITUATION: Just parsed symbol with class 'E', or special symbol MC.
 *	Parses the args putting values on the stack, arg pointing to the first 
 *	of them.
 *	Sets the cursor to the called function's 'where'. Parses arg decl's
 *	giving them values from the stack. Executes the function body.
 *	Pops the locals (vars and values). Restores the caller's stcurs and 
 *	cursor.
 */
void _enter( char* where) {
	int arg=nxtstack;
	int nargs=0;
	if(varargs>0) nargs=varargs-1;
	if(where)fcn_enter();
	_lit(xlpar); /* optional (   */
	int haveArgs = ! (  _lit(xrpar)
					 || *cursor==*xlb
					 || *cursor==*xrb
					 || *cursor==*xsemi
					 || *cursor==*xnl
					 || *cursor==0x0d
					 || *cursor==*xslash
					);
	if ( haveArgs ) {
		do {
			if(error)return;
			if( _asgn()) ++nargs;
			else break;  /* break on error */
		} while( _lit(xcomma) );
	}
	if(error)return;
	_lit(xrpar);   /* optional )   */
	_rem();
	if(!where) {
		if(nxtstack) {
			machinecall( nargs );
			varargs=0;
		}
		else eset(MCERR);
		return;
	}
/* ABOVE parses the call args, BELOW parses the called's arg decls.
 */
	else {
		newfun(locals);  
		char *localstcurs=stcurs, *localcurs=cursor;
		curfun->obj = curobj;
		curobj = canobj;
		cursor = where;
		for(;;) {	  
			_rem();
			if(_lit(xint)) { 
				do {
					_setArg(Int, &stack[arg],locals);
					arg++;
				} while(_lit(xcomma));
				_lit(xsemi); /* optional */
			} 
			else if ( _lit(xchar)) {
				do {
					_setArg(Char, &stack[arg],locals);
					arg++;
				} while(_lit(xcomma));
				_lit(xsemi);
			}
			else if ( _lit(xvarargs) ){
				varargs=nargs+1;
				break;
			}
			else {
				break;
			}
		}
		if(!varargs) {
			if(arg != nxtstack) {
				cursor=localcurs;
				stcurs=localstcurs;
				eset(ARGSERR);
			}
			while(nargs>0){
				popst();
				--nargs;
			}
		}
		if(!error)st();     /*  <<-- execute fcn's body */
		if(!leave)pushzero();
		leave=0;
		cursor=localcurs;
		stcurs=localstcurs;
		curobj = curfun->obj;
		fundone();
		fcn_leave();
	}
}

/*	parses constant defining fname..lname which brackets trimmed constant. 
 *	Cursor moved just beyond constant. Returns Type: 
 */
Type _konst() {
	char* x;
	while(*cursor==' ')++cursor;
	char c = *cursor;
	if( c=='+' || c=='-' || (c>='0'&&c<='9') ) {
		fname = cursor;
		do{
			++cursor; c=*cursor;
		} while(c>='0'&&c<='9');
		lname=cursor-1;
		if(verbose[VP]){
			fprintf(stderr,"\nparsed ");
			dumpft(fname,lname);
		}
		return Int;

	} else if(_lit("\"")) {
		fname=cursor;
		/* set lname = last char, cursor = lname+2 (past the quote) */
		if( (x = _findEOS(fname)) ) {
			lname = x-1; /*before the quote */
			cursor = x+1; /*after the quote */
			*x = 0;
		}
		else { eset(CURSERR); return Err; }
		if(verbose[VP]){
			fprintf(stderr,"\nparsed ");
			dumpft(fname,lname);
		}
		return CharStar;

	} else if(_lit("\'")) {
		fname=cursor;
		/* lname = last char, cursor = lname+2 (past the quote) */
		if( (x=_mustFind(fname+1,fname+2,'\'',CURSERR)) ) {
			lname = x-1; 
			cursor = x+1;
		}
		else { eset(CURSERR); return -1; }
		if(verbose[VP]){
			fprintf(stderr,"\nparsed ");
			dumpft(fname,lname);
		}
		return Char;
	
	} else return Err;  /* no match, Err==0 */
}

/* An asgn is a reln or an lvalue = asgn. Note that
   reln can match an lvalue.
 */
int _asgn(){ 
	if(_reln()){
		if(_lit(xeq)){
			_asgn();
			if(!error)_eq();
		}
	}
	return error? 0: 1;
}

/* a RELN is an expr or a comparison of exprs
 */
int _reln(){
	if(_expr()){
		if(_lit(xle)){
			if(_expr()){
				if(topdiff()<=0)pushone();
				else pushzero();
			}
		}
		else if(_lit(xge)){
			if(_expr()){
				if(topdiff()>=0)pushone();
				else pushzero();
			}
		}
		else if(_lit(xeqeq)){
			if(_expr()){
				if(topdiff()==0)pushone();
				else pushzero();
			}
		}
		else if(_lit(xnoteq)){
			if(_expr()){
				if(topdiff()!=0)pushone();
				else pushzero();
			}
		}
		else if(_lit(xgt)){
			if(_expr()){
				if(topdiff()>0)pushone();
				else pushzero();
			}
		}
		else if(_lit(xlt)){
			if(_expr()){
				if(topdiff()<0)pushone();
				else pushzero();
			}
		}
		else return 1;  /* just expr is a reln */
	}
	return 0;   /* not an expr is not a reln */
}

/* ;an EXPR is a term or sum (diff) of terms.
 */
int _expr(){
	if(_lit(xminus)){    /* unary minus */
		_term();
		pushk(-toptoi());
	}
	else if(_lit(xplus)){
		_term();
		pushk(toptoi());
	}
	else _term();
	while(!error){    /* rest of the terms */
		int leftclass = stack[nxtstack-1].class;
		int rightclass;
		if(_lit(xminus)){
			_term();
			rightclass = stack[nxtstack-1].class;
			ptrdiff_t b=toptoi();
			ptrdiff_t a=toptoi();
			if( rightclass || leftclass) pushPtr(a-b);
			else pushk(a-b);
		}
		else if(_lit(xplus)){
			_term();
			rightclass = stack[nxtstack-1].class;
			ptrdiff_t b=toptoi();
			ptrdiff_t a=toptoi();
			if( rightclass || leftclass) pushPtr(a+b);
			else pushk(a+b);
		}
		else return 1;   /* is expression, all terms done */
	}
	return 0;   /* error, set down deep */
}

/* ;a term is a factor or a product of factors.
 */
int _term() {
	_factor();
	while(!error) {
		if(_lit(xstar)){
			_factor();
			if(!error)pushk(toptoi()*toptoi());
		}
		else if(_lit(xslash)){
			if(*cursor=='*' || *cursor=='/') {
				--cursor;    /* opps, its a comment */
				return 1;
			}
			_factor();
			ptrdiff_t denom = toptoi();
			ptrdiff_t numer = toptoi();
			if(denom){
				ptrdiff_t div = numer/denom;
				if(!error)pushk(div);
			}
			else eset(DIVERR);
		}
		else if(_lit(xpcnt)){
			_factor();
			ptrdiff_t b=toptoi();
			ptrdiff_t a=toptoi();
			if(b){
				ptrdiff_t pct = a%b;
				if(!error)pushk(pct);
			}
			else eset(DIVERR);
		}
		else return 1;  /* no more factors */
	}
	return 0;
}

/*	Parse a required symname. Return its required entry in vh.
 *	Else appropriate error. Set curobj to qual's vh.
 */
struct var* obsym(char* qual) {
	struct var *qvar;
	struct varhdr *qvh;
	struct var *ovar;
	qvar = addrval_all(qual);
	if(!qvar){
		eset(SYMERR);
		return NULL;
	}
//fprintf(stderr,"\ntoc~587 qvar: %x",qvar);
//dumpVar(qvar);
	canobj = qvh = (struct varhdr*)qvar->vdcd.od.blob;
//fprintf(stderr,"\ntoc~590 qualifiers blob, %p",qvh);
//dumpBV(qvh);
	if(symName()){
		ovar = addr_obj(qvh);
		cursor = lname+1;
		if(!ovar) eset(SYMERR);
		return ovar;
	}
	else eset(SYNXERR);
	return NULL;
}   //was ~578

/* a FACTOR is a ( asgn ), or a constant, or a variable reference, or a function
    reference. NOTE: factor must succeed or it esets SYNXERR. Callers test error
    instead of a returned true/false. This varies from the rest of the expression 
    stack.
 */
void _factor() {
	struct var *isvar;
	Type type;
	char* x;
	if(_lit(xlpar)) {
		_asgn();
		if( (x=_mustFind( cursor, cursor+5, ')' , RPARERR )) ) {
			cursor = x+1; /*after the paren */
		}
	} 
	else if( (type=_konst()) ) {
	/* Defines fname,lname. Returns Type. */
		switch(type){
		case Int: 
			pushk( _atoi() );  /* integer, use private _atoi */
			break;
		case Char:
			foo.uc = *fname;
			pushst( 0, 'A', type, &foo );
			break;
		case CharStar:		/* special type used ONLY here */
			foo.up = fname;
			pushst( 1, 'A', Char, &foo );
		case Err:
			return;
		}
	}
	else if(_lit(xnew)){
//		_rem();
		struct var *isclvar;
		if((isclvar=_isClassName(NODOT))){
			struct varhdr *vh;
			vh = classlink(isclvar);
// call constructor (enter) if exist
			struct var *con = _addrval(isclvar->name,vh->vartab,(vh->nxtvar)-1);
			if(con){
				char *where = con->vdcd.vd.value.up;
				canobj = vh;   // enable instance variable search
				_enter(where);
				popst();      // pop constructor returned value
			}
			union stuff value;
			value.up = vh;
			pushst(0,'A','o',&value);
		}
		else eset(CLASSERR);
	}
	else if((isvar=_isClassName(YESDOT))) {
		if(*cursor=='.'){              // Game.play
			struct varhdr *vh;
			if(isvar->vdcd.cd.blob==NULL){
				isvar->vdcd.cd.blob = vh = classlink(isvar);
			}
			else vh = isvar->vdcd.cd.blob;
			++cursor;
			if(symName()){
				cursor=lname+1;
				struct var *v;
				v = addr_obj(vh);
				if(!v){eset(SYMERR);return;}
				//enter or push the value
			  	char* where = v->vdcd.vd.value.up;
				if(v->vdcd.vd.class=='E'){
					_enter(where);
				}
				else {
					pushvar(v);
				}
			}
			else eset(SYMERR);
		}
		else if(symName()) {           // decl of var of type 'o'
			cursor = lname+1;
			newref(isvar,locals);
		}
		else eset(SYMERR);
	}
	else if( symName() ) {
		cursor = lname+1;
		if( symNameIs("MC") ) { 
			_enter(0); return;
		} 
		else {
			struct var *v;
			if( *(lname+1)=='.' ) {  // obj qualifier
				struct var qual;
				canon(&qual);
				cursor = lname+2;
				v = obsym(qual.name);  /* looks up symbol */
			}
			else v = addrval();  /* looks up symbol */
			if( !v ){ eset(SYMERR); return; } /* no decl */
			if(v->type=='o'){  //object ref
				union stuff value;
				value.up = &(v->vdcd.od.blob);
				pushst(0,'L','o',&value);
				return;
			}
		  	char* where = (*v).vdcd.vd.value.up;
		  	int class=(*v).vdcd.vd.class; 
		  	if( class=='E' ) _enter(where);  /* fcn call */
		  	else pushvar(v);
		}
	}
	else {
		eset(SYNXERR);
	}
}
