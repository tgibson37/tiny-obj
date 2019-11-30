#include "toc.h"

int varargs = 0;
union stuff foo;

/* Situation: parsing argument declarations, passed values are on the stack.
 * arg points into stack to an argument of type. 
 * Gets actual value of arg, calls valloc which parses and sets
 * up local with the passed value.
 */ 
void _setArg( Type type, struct stackentry *arg, 
				struct varhdr *locals ) {
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
	varalloc( type, NULL, &vpassed, locals);
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

