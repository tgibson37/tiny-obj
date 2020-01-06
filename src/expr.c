#include "expr.h"
#include "factor.h"
#include "stack.h"
#include "var.h"
//#include "platform.h"
#include "toc.h"

union stuff foo;
int _reln();
int _expr();
int _term();

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

