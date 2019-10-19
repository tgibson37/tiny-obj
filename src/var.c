#include "toc.h"

int newop;

/*  fetch a value and type given its var. On success return len
 *  (or 1 for obj-ref) else 0. Details...
 *    Full retrieval into struct *stuff s for char or int.
 *    Pointer into val space for char* or int*.
 *    Pointer to blob for object ref.
 *  Used only for data, not for type 'E'  
 */
int fetchVal(struct var *v, union stuff *s){
    void *where = v->vdcd.vd.value.up;
	if(v->type == 1){
		s->uc = get_char(where);
		return v->vdcd.vd.len;
	}
	else if(v->type == 2){
		s->ui = get_int(where);
		return v->vdcd.vd.len;
	}
	else if(v->type == 'o'){
		s->up = v->vdcd.od.blob;
		return 1;
	}
	return 0;
}

/*	Print the value (a brief excerpt for arrays) described by *v.
 */
void dumpVal_v(struct var *v){
	union stuff s;
	int len = fetchVal(v, &s);
	if(v->type == 1){
		if(len>1)fprintf(stderr,"[%c, %c, ...]",s.uc,(s.uc)+1);
		else fprintf(stderr,"%c",s.uc);
	}
	else if(v->type == 2){
		if(len>1)fprintf(stderr,"[%d, %d, ...]",s.uc,(s.uc)+1);
		else fprintf(stderr,"%lld",(long long)s.ui);
	}
	else if(v->type == 'o'){
		struct varhdr *vh = s.up;
		struct blob *b;
		for(b=blobtab; b<nxtblob; ++b) {
			if(b->varhdr == vh->vartab->vdcd.od.blob) break;
		}
		if(b<nxtblob) fprintf(stderr,"%s",b->name);
		else fprintf(stderr,"Object type");
	}
	else fprintf(stderr,"Unknown type");
}

/* SITUATION: Function call parsed PLUS two calls during linking.
	Open new var and value frames for library, globals, and function locals.
 */
void dumpDots(int n){while(n--)fprintf(stderr,".");}

void newfun(struct varhdr *vh) {
	if(++curfun>efun){
		eset(TMFUERR);
	} 
	else {
		curfun->fvar = curfun->evar = vh->nxtvar;
		curfun->datused = vh->datused;
	}
//fprintf(stderr,"\nrep run %d %d  ",db_report_depth, db_rundepth);
	if(verbose[VF] && db_report_depth <= db_rundepth){
		fflush(stdout);
		fprintf(stderr,"\n");
		dumpDots(fcnDepth());
		fprintf(stderr,"calling: %s		",fcnName);
	}
}

/* SITUATION: function is completed. 
 *	Close its var and value frames.
 */
void fundone() {
	locals->nxtvar=curfun->fvar;
	locals->datused=curfun->datused;
	--curfun;
#if 0
	if(verbose[VF] && db_report_depth < db_rundepth){
		fflush(stdout);
		fprintf(stderr,"\n");
		dumpDots(fcnDepth());
		fprintf(stderr,"fundone");
	}
#endif
}

/*********** var tools ****************/

/*	copy the argument value into the new local place.
 */
int _copyArgValue(struct var *v, int class, Type type, union stuff *passed ) {
	if(passed && class){   					/* passed pointer */
		(*v).vdcd.vd.value.up = (*passed).up;
	} else if( passed && !class ) {			/* passed datum */
		switch(type){
		case Int:
			put_int( (*v).vdcd.vd.value.up, (*passed).ui );
			break;
		case Char:
			put_char( (*v).vdcd.vd.value.up, (*passed).uc );
			break;
		default:
			eset(TYPEERR);
			return TYPEERR;
		}
	}
	return 0;
}

/* allocates memory for value of v, return 0 on success, else !0
 */
int _allocSpace(struct var *v, int amount, struct varhdr *vh){
	if( vh->datused+amount > vh->endval ) {
		eset(TMVLERR);
		return TMVLERR;
	}
	v->vdcd.vd.value.up = vh->datused;
	memset( vh->datused, 0, amount );
	vh->datused += amount;
	return 0;
}
/* SITUATION: Declaration is parsed, and its descriptive data known.
 * 	Fill in the var with this data. Allocate value storage unless already
 *	allocated, i.e. pointer to passed data. Copy passed data into allocation.
 *	NOTE: signifantly refactored.
 */
void newvar( int class, Type type, int len, union stuff *passed, struct varhdr *vh ) {
	int obsize = typeToSize(class,type);
	if(vh==NULL){
		lndata.nvars += 1;
		lndata.valsize += len*obsize;
		return;
	}
	struct var *v = vh->nxtvar;
	struct var *evar = (struct var*)vh->val;
	canon(v);    /* sets the canon'd name into v */
	(*v).vdcd.vd.class = class;
	(*v).type = type;
	(*v).vdcd.vd.len = len;
	(*v).vdcd.vd.brkpt = 0;
	if(_allocSpace(v,len*obsize,vh)) return;  /* true is bad, eset done */
	if(passed) _copyArgValue( v, class, type, passed);
	if(curfun>=fun) curfun->evar = vh->nxtvar;
	if( vh->nxtvar++ >= evar )eset(TMVRERR);
	if(verbose[VV])dumpVar(v);
	return;
}
void cls_dcl(int abst,char *cname,char *ename,struct varhdr *vh, char* where){
//dumpBlob(vh);
	struct var *c = vh->nxtvar++;
	strcpy(c->name,cname);
	c->type = abst?'A':'C';
	strncpy(c->vdcd.cd.parent,ename,VLEN);
	c->vdcd.cd.where = where;
//dumpVar(c);
}

/*  Refenence to an object: refname (fname,lname), type 'o', 
 *  details: class entry (cls), blob to referenced object's blob (NULL) 
 *  to be filled in when known.
 */
void newref(struct var *cls, struct varhdr *vh) {
  struct var *r = vh->nxtvar;
  canon(r);
  r->type = 'o';
  r->vdcd.od.cls = cls;
  r->vdcd.od.blob=NULL;
  vh->nxtvar++;
}

/* Canonicalizes the name bracket by f,l inclusive into buff, and returns buff.
	sizeOf buff must be at least VLEN+1.
 */
char* _canon(char* first, char* l, char* buff) {
	int i=0; 
	char* f=first;
	while(f<=l && i<VLEN-1) buff[i++]=*(f++);
	if(f<=l)buff[i++]=*l;
	buff[i]=0;
	return buff;
}

/* 	fname..lname is full name. Puts canonicalized name into v. If short
 *	enough same as name. If ptrdiff_ter first VLEN-1 characters + last character.
 *	The last char has more info than middle chars.
 */
void canon(struct var *v) {
	_canon(fname,lname,(*v).name);
}
/*	if *cursor is alphanum find end of sym and canon into buff
 */
int canonIf(char* buff){
	char *c=cursor;
	while(isalnum(*c)||*c=='_')++c;
	_canon(cursor,c-1,buff);
	return (c-cursor);
}
/*	returns var* defining current sym
 */
struct var* addr_obj(struct varhdr *vh){
	struct var sym;
	canon(&sym);
	return _addrval(sym.name,vh->vartab, vh->nxtvar-1);
}
/* 	looks up a symbol at one level
 */
struct var* _addrval(char *sym, struct var *first, struct var *last) {
	struct var *pvar;
	for(pvar=first; pvar<=last; ++pvar) {
		if( !strcmp(pvar->name, sym) ) {
			if( debug && (pvar->vdcd.vd.brkpt==1) )br_hit(pvar);
			return pvar;
		}
	}
	return NULL;
}

/* 	looks up a symbol at three levels via function table. 
 *	sym must be canonical.
 */
struct var* addrval_all(char *sym) {
	struct var *v;
	v = _addrval( sym, curfun->fvar, curfun->evar );  // locals
	if(!v && curobj)                                  // instances
			v = _addrval( sym, curobj->vartab, curobj->nxtvar-1);
	if(!v) v = _addrval( sym, curglbl->fvar, curglbl->evar ); //globals
	if(!v) v = _addrval( sym, fun->fvar,fun->evar );  //libs
	if(v)return v;
	return 0;	
}

/* 	looks up a symbol pointed to by fname,lname: 
 *	locals, globals, library levels in that order. First hit wins.
 */
struct var* addrval() {
	struct var sym;
	canon( &sym );
	return addrval_all(sym.name);
}

/* cursor points to possible sym. If it is a class name
 *	bump the cursor and return var entry
 */
struct var* _isClassName() {
	char buf[VLEN+1];
	int len = canonIf(buf);
	if(len){
		struct var *maybe = _addrval(buf,curglbl->fvar, curglbl->evar);
		if(maybe){
			int t = maybe->type;
			if(t=='C' || t=='A') {
				cursor += len;
				return maybe;
			}
		}
		return NULL;
	}
	return NULL;
}

/*	prints a value given its description taken from a struct stackEntry */
void dumpVal(Type t, int class, union stuff *val, char lval){
	fprintf(stderr,"pr[%d]",(int)((*val).up-(void*)pr));
	if(class==1 && t==Char ){
		char sval[30];
		strncpy(sval, (char*)((*val).up), 30);
		fprintf(stderr,"->%s<-", sval);
	}
	else if(class==0 && lval!='A'){ 
		if(t==Char){
			char x = *(char*)((*val).up);
			if(x)fprintf(stderr,"->%c<-", x );
			else fprintf(stderr,"->NULL<-");
		}
//		else fprintf(stderr,"->%d<-", (*val).ui );
		else fprintf(stderr,"->%d<-", *(int*)((*val).up) );
	}
/*
	else if(t==Char) fprintf(stderr,"%c",(*val).uc);
	else fprintf(stderr,"%d",(*val).ui);
*/
}

void dumpFunEntry( int e ) {
	fprintf(stderr,"\n fun entry at %d:  %p %p %p", e,
		fun[e].fvar, fun[e].evar, fun[e].datused );
}

void dumpFun() {
	fprintf(stderr,"\nfun table: fvar, evar, prused");
	int i;
	int num = curfun-fun;
	for(i=0;i<=num;++i) {
		dumpFunEntry(i);
	}
}

char *__typwrd_0__ = "Actual";
char *__typwrd_1__ = "Pointer";
char *__typwrd_o__ = "Object";
char *__typwrd_E__ = "Function";
char *__typwrd_Uk__ = "Unknown Type";

char* classToWord(struct var *v){
	switch((*v).vdcd.vd.class) {
		case 0: return __typwrd_0__;
		case 1: return __typwrd_1__;
		case 0x6f: return __typwrd_o__;
		case 0x45: return __typwrd_E__;
		default: return __typwrd_Uk__;
	}
}

void dumpVar(struct var *v) {
	if(v->type=='A' || v->type=='C') {
		fprintf(stderr,"\n class %s type %c ",v->name, v->type);
		if(*(v->vdcd.cd.parent))fprintf(stderr,"extends %s ", v->vdcd.cd.parent);
	}
	else if(v->type=='o') {
		fprintf(stderr,"\n oref: %s type %c classvar %p (%s) blob %p "
				,v->name, v->type,v->vdcd.od.cls, v->vdcd.od.cls->name
				,v->vdcd.od.blob);
	}
	else {
		fprintf(stderr,"\n var %p: %s %s %s len %d %zd %p "
			, v, (*v).name, classToWord(v), typeToWord((*v).type)
			, (*v).vdcd.vd.len, (*v).vdcd.vd.value.ui
			, (*v).vdcd.vd.value.up );
	}
	dumpVal_v(v);
}

void dumpVarTab(struct varhdr *vh) {
	int pos = 0;
	fprintf(stderr
			,"\nVar Table: name class type len value (as int and ptr)");
	struct var *v = vh->vartab;
	while(v < vh->nxtvar) {
		dumpVar(v++);
		++pos;
	};
	if( !pos )fprintf(stderr,"\nempty\n");
	else fprintf(stderr,"\n");
}

void dumpBlob(struct varhdr *vh){
	fprintf(stderr,"\nBlob (aka varhdr) at %p \n",vh );
	struct var *vt = vh->vartab;
	char* dt = vh->val;
	fprintf(stderr,"  vartab       nxtvar    gltab     evar(val)\n");
	fprintf(stderr,"   %9p %9zd %9zd %9zd\n",vh->vartab
		,(ptrdiff_t)vh->nxtvar-(ptrdiff_t)vt,(ptrdiff_t)vh->gltab-(ptrdiff_t)vt
		,(ptrdiff_t)vh->val-(ptrdiff_t)vt);
	fprintf(stderr,"  val          used      endval\n");
	fprintf(stderr,"   %9p %9zd %9zd\n",vh->val
			,(ptrdiff_t)vh->datused-(ptrdiff_t)dt
			,(ptrdiff_t)vh->endval-(ptrdiff_t)dt);
	fprintf(stderr,"nxtvar,gltab are decimal sizes in vartab");
	fprintf(stderr,"\nused,endval are decimal sizes in/of val");
}
void dumpBlobTab() {
	struct blob *b;
	fprintf(stderr,"\nvvv blob table vvv\n");
	for(b=blobtab; b<nxtblob; ++b) {
		fprintf(stderr,"\n%s: ",b->name);
		dumpBlob(b->varhdr);
		fprintf(stderr,"\n");
	}
	fprintf(stderr,"\n^^^ blob table ^^^\n");
}

void dumpBV(struct varhdr *vh){ 
	dumpBlob(vh); 
	dumpVarTab(vh);
}

/*	_newblob,newblob,_getblob,getblob build and search the blob table
 */
// Enters blob into blobtab. 
void _newblob(char* name, void* blob){
	if(nxtblob >= eblob){eset(TMBLOBERR);return;}
	struct blob *b = nxtblob++;
	strcpy(b->name,name);  //strlen(name) must be <= VLEN
	b->varhdr = blob;
}

//Assumes sym is canonicalized
struct varhdr* _getblob(char* sym){
	struct blob *b;
	for(b=blobtab; b<nxtblob; ++b) {
    	if( !strcmp(b->name, sym) ) {
    		return b->varhdr;
    	}
  	}
  	return NULL;
}

/*	A class blob is named the same as the class whose vars it defines.
 *	Globals and locals are named __Globals__ and __Locals__ resp.
 *	Assumes symName() has just parsed and defined fname,lname.
 */
struct varhdr* getblob(){
  struct var sym;
  canon( &sym );
  return _getblob(sym.name);
}

void getBlobName(struct varhdr *vh){

}

/*	Checks for balanced brackets, from *from to *to.
 */
int checkBrackets(char *from, char *to) {
	int err;
	char* savedCursor=cursor;
	char* savedEndapp=endapp;
	cursor = from;
	endapp = to;
	while(cursor<endapp) {
		while(*(cursor++) != '[' && cursor<endapp) ;
		if(cursor<endapp) {
			if( (err=_skip('[',']')) )return err;
		}
	}
	cursor = savedCursor;
	endapp = savedEndapp;
	return 0;
}

int xxpass=0;

/*	Pass one if varhdr is NULL computes the needed sizes. Pass two does
 *	the actual link into varhdr, which also has room for all values.
 *	The logic here mimics classical void link().
 */
void lnpass12(char *from, char *to, struct varhdr *vh, int newop) {
	char* cptr;
	char* savedEndapp=endapp;
	char* savedCursor=cursor;
//	struct var *vartab;
	if(vh==NULL){
//		vartab=NULL;   // pass 1 
		xxpass=1;
	} else {
//		vartab=vh->vartab;
		xxpass=2;
		if(!newop)newfun(vh);
	}
	if(checkBrackets(from,to))eset(RBRCERR+1000);
	if(error){ whatHappened(); exit(1); }
	cursor=from;
	endapp=to;
	while(cursor<endapp && !error){
		char* lastcur = cursor;
		_rem();
		if(_lit(xlb)) _skip('[',']');
		else if( _decl(vh) ) ;
		else if( _lit(xendlib) ){
			if(vh != NULL){     //  <<==  PASS TWO endlibrary
				if(curfun==fun) {   /* 1st endlib, assume app globals follow */
					if(!newop){
						newfun(vh);
						curglbl=curfun;
					}
				}
				else {        // multiple endlib tolerance, undo the assumption
					fun[0].evar = fun[1].fvar = vh->nxtvar;
				}
				vh->gltab = vh->nxtvar;  // start (or start over) on fun[1]
			}
		}
		else if(_lit(xclass)||_lit(xabstract)){
			struct var cname, ename;
			int abst=0;
			char *where;
			if( *(cursor-1)=='t') {
				abst=1;
				if(_lit(xclass)) ;
				else eset(SYNXERR);
			}
			if(symName()) {     /* class name */
//				union stuff kursor;
//				kursor.up = 
				cursor = lname+1;
				canon(&cname);
				if(_lit(xextends)){
					if(symName()){   // parent name
						cursor=lname+1;
						canon(&ename);
					}
					else eset(SYNXERR);
				}
				_rem();
				where = cursor;
			}
			else {
				eset(SYNXERR);
				return;
			}
			if(xxpass==1){
				lndata.nvars += 1;
			}
			else if(xxpass==2){
				cls_dcl(abst,cname.name,ename.name,vh,where);
			}
		}
		else if(symName()) {     /* fctn decl */
			union stuff kursor;
			cursor = lname+1;
			_rem();
			kursor.up = cursor;
			newvar('E',2,0,&kursor,vh);
			if( (cptr=_mustFind(cursor, endapp, '[',LBRCERR)) ) {
				cursor=cptr+1;
				_skip('[',']');
			}
		}
		else if(*cursor=='#'){
			while(++cursor<endapp) {
				if( (*cursor==0x0d)||(*cursor=='\n') )break;
			}
		}
		if(cursor==lastcur){
			eset(LINKERR);
		}
	}
	cursor = savedCursor;
	endapp = savedEndapp;
	if(verbose[VL])dumpVarTab(vh);
}

/*	build its vartab, make entry into owner's vartab
 */
void classlink(struct var *vh){
	return;   // model for this in Resources link.c/h
}

/*	tools for accessing specific data in a *var. A var can be
 *	id'd two ways: ptr to the var entry (struct var *v), and
 *	symbol name used to install that entry. These use the former.
 *	The latter requires	a search, and that search needs to know 
 *	what vartab to search. cname_to_var() searches globals for class name.
 */

/*	Return var if name is a class, else NULL
 */
struct var *visclass(char *name){
	struct var* v = _addrval( name, curglbl->fvar, curglbl->evar );
	if(!v)return NULL;
	int t = v->type;
	if(t=='C' || t=='A') return v;
	return NULL;
}
char* vname(struct var *v) {
	return v->name;
}
int vtype(struct var *v){
	return (int)v->type;
}
/* v must be a class entry. Returns ptr to string, parent name. */
char* vparent(struct var *v) {
	return v->vdcd.cd.parent;
}
char* vwhere(struct var *v){
	return v->vdcd.cd.where;
}

/*	return length of body of class definition,
 *	incl lead [, excl trail ]. So an empty body [] has length 1.
 *	Return NULL if bracket problem. Note that where MUST point to [.
 */
int class_body(char* name){
	char *f;
	int len=0;
	struct var *v = visclass(name);
	char* saved_cursor = cursor;
	f = cursor = vwhere(v);
	if(*cursor != '[')eset(WHERERR);
	++cursor;
	if(_skip('[',']'))eset(RBRCERR);
	else len = cursor-f-1;
	cursor = saved_cursor;
	if(error)return 0;
	return len;
}

char par_buf[VLEN+2];

/*	ascend is used by lnlink to climb one step up the
 *	inheritance chain and set from/to to the text of
 *	the parents body, inside the []'s. The new object's
 *	vartab thus includes all inherited declarations.
 *	Callers arg from==zero signals no parent to stop the loop.
 */
void ascend(char *cname, char **from, char **to){
	struct var *v = visclass(cname);
	if(v){
		strcpy(par_buf,vparent(v));
		v = visclass(par_buf);
		if(v){
			*from = vwhere(v)+1;
			int len = class_body(par_buf);
			*to = *from+len-1;
		}
		else *from=0;
	}
	else *from=0;	
}

/*	Modified version of tclink() to parse and build class tables. 
 *	Links source text area from *from to *to in two passes. In pass one 
 *	vartab is NULL and computes the size of memory needed for both the 
 *	vartab and the value space. In pass two it builds the table. 
 *	Parse, var, and other services are supplied by whole unchanged tiny-c files.
 *	Uses lnpass12 as a service.
 */
struct varhdr* lnlink(char *from, char *to, char *blobName){
        newop = strcmp(blobName,"__Globals__"); // true iff doing new operator
        int size;
        char* blob;
        struct varhdr *vh;
        char* savedcursor=cursor;
        char* savedendapp=endapp;
        lndata.nvars = lndata.valsize = 0;
        char *f; char *t;
        f=from; t=to;
		strncpy(par_buf,blobName,VLEN+1);
        while(f){
	        lnpass12(f,t,NULL,newop);    // PASS one
	        if(newop){
	        	ascend(par_buf,&f,&t);
	        }
	        else break;
        }
        size = sizeof(struct varhdr) + lndata.nvars*sizeof(struct var) + lndata.valsize;
        blob = mymalloc("blob", size+10);
        vh = (struct varhdr*)blob;
        _newblob(blobName,blob);
		memset(vh, 0, size);
        vh->vartab = vh->nxtvar = vh->gltab = (struct var*)(vh+1);
        vh->val = vh->datused = (char*)(vh->vartab+lndata.nvars);
        vh->endval = blob + size;
        f=from; t=to;
		strncpy(par_buf,blobName,VLEN+1);
        while(f){
	        lnpass12(f,t,vh,newop);    // PASS two
	        if(newop){
	        	ascend(par_buf,&f,&t);
	        }
	        else break;
        }
		strncpy(par_buf,blobName,VLEN+1);  // just in case
        cursor=savedcursor;
        endapp=savedendapp;
        return vh;
}

/* links the loaded program. Uses cursor and endapp globals 
 *	which are set by the loader. Returns pointer to vartab, whose
 *	value references point to space in the same malloc.
 */
void toclink() {
	lnlink(cursor,endapp,"__Globals__");
}

#if 0
// useful code lines...
if(newop){

fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
dumpBlobTab();
dumpBlob(blob);
}
#endif