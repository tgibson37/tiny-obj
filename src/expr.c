#include "expr.h"
#include "factor.h"
#include "stack.h"
#include "var.h"
#include "platform.h"
#include "toc.h"

union stuff foo;

#if 0
//int varargs = 0;   //moved to factor.c
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
			if( _asgn()) ++nargs;
			else break;  /* break on error */
		} while( lit(xcomma) );
	}
	if(error)return;
	lit(xrpar);   /* optional )   */
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
			if(lit(xint)) { 
				do {
					_setArg(Int, &stack[arg],locals);
					arg++;
				} while(lit(xcomma));
				lit(xsemi); /* optional */
			} 
			else if ( lit(xchar)) {
				do {
					_setArg(Char, &stack[arg],locals);
					arg++;
				} while(lit(xcomma));
				lit(xsemi);
			}
			else if ( lit(xvarargs) ){
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
#endif

/* An asgn is a reln or an lvalue = asgn. Note that
   reln can match an lvalue.
 */
int asgn(){ 
	if(_reln()){
		if(lit(xeq)){
			asgn();
			if(!error)_eq();
		}
	}
	return error? 0: 1;
}

/* a RELN is an expr or a comparison of exprs
 */
int _reln(){
	if(_expr()){
		if(lit(xle)){
			if(_expr()){
				if(topdiff()<=0)pushone();
				else pushzero();
			}
		}
		else if(lit(xge)){
			if(_expr()){
				if(topdiff()>=0)pushone();
				else pushzero();
			}
		}
		else if(lit(xeqeq)){
			if(_expr()){
				if(topdiff()==0)pushone();
				else pushzero();
			}
		}
		else if(lit(xnoteq)){
			if(_expr()){
				if(topdiff()!=0)pushone();
				else pushzero();
			}
		}
		else if(lit(xgt)){
			if(_expr()){
				if(topdiff()>0)pushone();
				else pushzero();
			}
		}
		else if(lit(xlt)){
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
	if(lit(xminus)){    /* unary minus */
		_term();
		pushk(-toptoi());
	}
	else if(lit(xplus)){
		_term();
		pushk(toptoi());
	}
	else _term();
	while(!error){    /* rest of the terms */
		int leftclass = stack[nxtstack-1].class;
		int rightclass;
		if(lit(xminus)){
			_term();
			rightclass = stack[nxtstack-1].class;
			ptrdiff_t b=toptoi();
			ptrdiff_t a=toptoi();
			if( rightclass || leftclass) pushPtr(a-b);
			else pushk(a-b);
		}
		else if(lit(xplus)){
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
	factor();
	while(!error) {
		if(lit(xstar)){
			factor();
			if(!error)pushk(toptoi()*toptoi());
		}
		else if(lit(xslash)){
			if(*cursor=='*' || *cursor=='/') {
				--cursor;    /* opps, its a comment */
				return 1;
			}
			factor();
			ptrdiff_t denom = toptoi();
			ptrdiff_t numer = toptoi();
			if(denom){
				ptrdiff_t div = numer/denom;
				if(!error)pushk(div);
			}
			else eset(DIVERR);
		}
		else if(lit(xpcnt)){
			factor();
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

