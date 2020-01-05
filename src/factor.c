#include "factor.h"
#include "expr.h"
#include "stack.h"
#include "var.h"
#include "link.h"
#include "toc.h"

/*	Situation: An object qualifier including its dot is parsed.
 *	Action: Parse the required symname. Return its struct var.
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


/*	Situation: v describes a variable to be pushed as an 
 *	lvalue onto the stack. Only the name is parsed. 
 *	Action: If subscripted finish parsing and resolve to 
 *	a specific element. Push as an lvalue.
 */ 
void _pushvar(struct var *v){
	union stuff foo;
	int class=v->vdcd.vd.class; 
	int type=v->type; 
	int obsize = typeToSize(class,type);
	char* where = v->vdcd.vd.value.up;
	int len=v->vdcd.vd.len;
	if( lit(xlpar) ) {		       /* is dimensioned */
  		if( !class ) {   /* must be class>0 */
			eset(CLASERR);
  		} else {  /* dereference the lvalue */
  			/* reduce the class and recompute obsize */
				obsize = typeToSize(--class,type);
  			/* increment where by subscript*obsize */
    		asgn(); if( error )return;
    		lit(xrpar);
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

/* Situation: parsing argument declarations, passed values are on the stack.
 * arg points into stack to an argument of type. 
 * Gets actual value of arg, calls valloc which parses and sets
 * up local with the passed value.
 */ 
void _setArg( Type type, struct stackentry *arg ) {
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
//fprintf(stderr,"\n~295SA type %d passed %d", type, vpassed.ui);
//	varAlloc( type, &vpassed);
	varalloc( type, NULL, &vpassed, locals);
}

/*	SITUATION: Just parsed symbol with class 'E', or special symbol MC.
 *	Parses the args putting values are on the stack, arg pointing to the first 
 *	of them.
 *	Sets the cursor to the called function's 'where'. Parses arg decl's
 *	giving them values from the stack. Executes the function body.
 *	Pops the locals (vars and values). Restores the caller's stcurs and 
 *	cursor.
 */
int varargs = 0;
char* xvarargs = "...";
void _enter( char* where) {
	int arg=nxtstack;
	int nargs=0;
	if(varargs>0) nargs=varargs-1;
//fprintf(stderr,"\n~311E ABOVE: va %d na %d",varargs,nargs);
	if(where)fcn_enter();
	lit(xlpar); /* optional (   */
	int haveArgs = ! (  lit(xrpar)
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
			if( asgn()) ++nargs;
			else break;  /* break on error */
		} while( lit(xcomma) );
	}
	if(error)return;
	lit(xrpar);   /* optional )   */
	rem();
	if(!where) {
		if(nxtstack) {
			machinecall( nargs );
			varargs=0;
//fprintf(stderr,"\n~336E va %d na %d",varargs,nargs);
		}
		else eset(MCERR);
		return;
	}
	else {   /* ABOVE parses the call args, BELOW parses the called's arg decls */
//fprintf(stderr,"\n~342E BELOW: va %d na %d",varargs,nargs);
		newfun(locals);
		char *localstcurs=stcurs, *localcurs=cursor;
		curfun->obj = curobj;
		curobj = canobj;
		cursor = where;
		for(;;) {	  
			rem();
			if(lit(xint)) { 
				do {
					_setArg(Int, &stack[arg]);
					arg++;
				} while(lit(xcomma));
				lit(xsemi); /* optional */
			} 
			else if ( lit(xchar)) {
				do {
//fprintf(stderr," ~358CHAR ");
					_setArg(Char, &stack[arg]);
					arg++;
				} while(lit(xcomma));
				lit(xsemi);
			}
			else if ( lit(xvarargs) ){
				varargs=nargs+1;
//fprintf(stderr,"\n~362E va %d na %d ",varargs,nargs);
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

/* same as find but sets err on no match */
char* mustFind( char *from, char *upto, char c, int err ) {
	char* x = find(from, upto, c);
	if( x ) return x;
	else { eset(err); return 0; }
}

/* return true if symname matches arg, no state change */
int symNameIs(char* name){
	int x = strncmp(fname, name, lname-fname+1);
	return( !x );
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

	} else if(lit("\"")) {
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

	} else if(lit("\'")) {
		fname=cursor;
		/* lname = last char, cursor = lname+2 (past the quote) */
		if( (x=mustFind(fname+1,fname+2,'\'',CURSERR)) ) {
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

/* a FACTOR is a ( asgn ), or a constant, or a variable reference, or a function
    reference. NOTE: factor must succeed or it esets SYNXERR. Callers test error
    instead of a returned true/false. This varies from the rest of the expression 
    stack.
 */
//int dmy=0;
//int foobar(){return 0;}
void factor() {
//if((++dmy)==73) dmy=foobar();
	union stuff foo;
	Type type;
	char* x;
	if(lit(xlpar)) {
		asgn();
		if( (x=mustFind( cursor, cursor+5, ')' , RPARERR )) ) {
			cursor = x+1; /*after the paren */
		}
	} 
	else if( (type=_konst()) ) {
	/* Defines fname,lname. Returns Type. 
	   void pushst( int class, int lvalue, Type type, void* stuff );
	*/
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
	else if(lit(xnew)){
//		_rem();
		struct var *isclvar;
		if((isclvar=_isClassName(NODOT))){
			struct varhdr *vh;
			vh = classlink(isclvar);
//fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
//dumpVarTab(vh);

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
                        else _pushvar(v);
                }
        }
	else {
		eset(SYNXERR);
	}
}

#if 0
	else if((isvar=_isClassName(MAYBEDOT))) {
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
					_pushvar(v);
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
			_enter(NULL); return;
		} else {
			struct var *v = addrval();  /* looks up symbol */
			if( !v ){ eset(SYMERR); return; } /* no decl */
		  	char* where = (*v).vdcd.vd.value.up;
//		  	int integer =  (*v).vdcd.vd.value.ui; 
//		  	int character = (*v).vdcd.vd.value.uc;
		  	int class=(*v).vdcd.vd.class; 
	  		int type=(*v).type; 
	  		int obsize = typeToSize(class,type);
	  		int len=(*v).vdcd.vd.len;
		  	if( class=='E' ) _enter(where);  /* fcn call */
			else {   /* is var name */
				if( lit(xlpar) ) {		       /* is dimensioned */
			  		if( !class ) {   /* must be class>0 */
						eset(CLASERR);
			  		} else {  /* dereference the lvalue */
			  			/* reduce the class and recompute obsize */
	  					obsize = typeToSize(--class,type);
			  			/* increment where by subscript*obsize */
		        		asgn(); if( error )return;
		        		lit(xrpar);
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
		}
	}
#endif
