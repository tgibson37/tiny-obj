#include "expr.h"
#include "factor.h"
#include "stack.h"
#include "var.h"
#include "platform.h"
#include "tc.h"

/* SITUATION: Parsed an assignment expression. Two stack entries, lvalue, datam.
 *	Effects the assignment. 
 */
void _eq() {
	DATINT  iDatum;  /* memcpy into these from pr using val.stuff */
	char cDatum;  /*  and val.size, giving needed cast */
	void* pDatum;
	void* where;

	struct stackentry *val = &stack[nxtstack-1]; /* value (on top) */
	struct stackentry *lval = &stack[nxtstack-2]; /* where to put it */
	if(verbose[VE]){
		fprintf(stderr,"\neq: lval");
		dumpStackEntry(nxtstack-2);
		fprintf(stderr,"\neq: val");
		dumpStackEntry(nxtstack-1);
	}
	popst();popst();  
	where = &((*lval).value.up);
	int class = (*lval).class;
	int type = (*lval).type;
//	int whereSize = typeToSize(class,type);  /* of the lvalue */

	if((*lval).lvalue != 'L') { 
		eset(LVALERR); 
		return; 
	}
	
	if(class==1 && (*val).class==1) {
		pDatum = (*val).value.up;
		if( (*val).lvalue=='L' ){
			pDatum = (char*)(*(int*)pDatum);   /* now its 'A' */
		}
		char **where = (*lval).value.up;
		*where = (char*)pDatum;
		pushst(class, 'A', type, &(*val).value);
	}
	else if(class==1 && (*val).class==0) {  /* ptr = int */
		if( (*val).type != Int ){
			eset(EQERR);
			return;
		}
		if( (*val).lvalue=='L' ) {
			iDatum = get_int((*val).value.up);
		}
		else {
			iDatum = (*val).value.ui;
		}
		pDatum = (void*)iDatum;
		char **where = (*lval).value.up;
		*where = (char*)pDatum;
		pushst(class, 'A', type, &(*val).value);
	}
	else if(class==0 && (*val).class==1) {  /* int = ptr */
		if(type!=Int){
			eset(EQERR);
			return;
		}
		pDatum = (*val).value.up;
		if( (*val).lvalue=='L' ){
			pDatum = (char*)(*(DATINT*)pDatum);   /* now its 'A' */
		}
		iDatum = (DATINT)pDatum;
		put_int( (*lval).value.up, iDatum);
		pushk(iDatum);
	}
	else if(class==0 && (*val).class==0) {
		if(type==Int){
			if( (*val).lvalue=='L' ) {
				iDatum = get_int((*val).value.up);
			}
			else {
				iDatum = (*val).value.ui;
			}
			if((*val).type==Char) iDatum = iDatum&0xff;
			put_int( (*lval).value.up, iDatum);
			pushk(iDatum);
		}
		else if(type==Char){
			if( (*val).lvalue=='L' ) {
				cDatum = get_char((*val).value.up);
			}
			else {
				cDatum = (*val).value.uc;
			}
			put_char( (*lval).value.up, cDatum );
			pushk(cDatum);
		}
	}
}

/********** expression parser *****************/

/* a term is a factor or a product of factors.
 */
int term() {
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
			DATINT denom = toptoi();
			DATINT numer = toptoi();
			if(denom){
				DATINT div = numer/denom;
				if(!error)pushk(div);
			}
			else eset(DIVERR);
		}
		else if(lit(xpcnt)){
			factor();
			DATINT b=toptoi();
			DATINT a=toptoi();
			if(b){
				DATINT pct = a%b;
				if(!error)pushk(pct);
			}
			else eset(DIVERR);
		}
		else return 1;  /* no more factors */
	}
	return 0;
}

/* an EXPR is a term or sum (diff) of terms.
 */
int expr(){
	if(lit(xminus)){    /* unary minus */
		term();
		pushk(-toptoi());
	}
	else if(lit(xplus)){
		term();
		pushk(toptoi());
	}
	else term();
	while(!error){    /* rest of the terms */
		int leftclass = stack[nxtstack-1].class;
		int rightclass;
		if(lit(xminus)){
			term();
			rightclass = stack[nxtstack-1].class;
			DATINT b=toptoi();
			DATINT a=toptoi();
			if( rightclass || leftclass) pushPtr(a-b);
			else pushk(a-b);
		}
		else if(lit(xplus)){
			term();
			rightclass = stack[nxtstack-1].class;
			DATINT b=toptoi();
			DATINT a=toptoi();
			if( rightclass || leftclass) pushPtr(a+b);
			else pushk(a+b);
		}
		else return 1;   /* is expression, all terms done */
	}
	return 0;   /* error, set down deep */
}

/* a RELN is an expr or a comparison of exprs
 */
int reln(){
	if(expr()){
		if(lit(xle)){
			if(expr()){
				if(topdiff()<=0)pushone();
				else pushzero();
			}
		}
		else if(lit(xge)){
			if(expr()){
				if(topdiff()>=0)pushone();
				else pushzero();
			}
		}
		else if(lit(xeqeq)){
			if(expr()){
				if(topdiff()==0)pushone();
				else pushzero();
			}
		}
		else if(lit(xnoteq)){
			if(expr()){
				if(topdiff()!=0)pushone();
				else pushzero();
			}
		}
		else if(lit(xgt)){
			if(expr()){
				if(topdiff()>0)pushone();
				else pushzero();
			}
		}
		else if(lit(xlt)){
			if(expr()){
				if(topdiff()<0)pushone();
				else pushzero();
			}
		}
		else return 1;  /* just expr is a reln */
	}
	return 0;   /* not an expr is not a reln */
}

/* An ASGN is a reln or an lvalue = asgn.
 */
int asgn(){ 
	if(reln()){
		if(lit(xeq)){
			asgn();
			if(!error)_eq();
		}
	}
	return error? 0: 1;
}
