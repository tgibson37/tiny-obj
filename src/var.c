#include "toc.h"

int newop;

/* SITUATION: Function call parsed PLUS two calls during linking.
	Open new var and value frames for library, globals, and function locals.
 */
void newfun(struct varhdr *vh) {
	if(++curfun>efun){
		eset(TMFUERR);
	} 
	else {
		curfun->fvar = curfun->evar = vh->nxtvar;
		curfun->datused = vh->datused;
	}
	if(verbose[VV]){
		fflush(stdout);
		fprintf(stderr,"\nnewfun %s",fcnName);
	}
}

/* SITUATION: function is completed. 
 *	Close its var and value frames.
 */
void fundone() {
	locals->nxtvar=curfun->fvar;
	locals->datused=curfun->datused;
	--curfun;
	if(verbose[VV]){
		fflush(stdout);
		fprintf(stderr,"\nfundone %s",fcnName);
	}
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
	canon(v);    /* sets the canon'd name into v */
	(*v).vdcd.vd.class = class;
	(*v).type = type;
	(*v).vdcd.vd.len = len;
	(*v).vdcd.vd.brkpt = 0;
	if(_allocSpace(v,len*obsize,vh)) return;  /* true is bad, eset done */
	if(passed) _copyArgValue( v, class, type, passed);
	if(curfun>=fun) curfun->evar = vh->nxtvar;
	if( vh->nxtvar++ >= vh->val )eset(TMVRERR);
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
 *	enough same as name. If longer first VLEN-1 characters + last character.
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

struct var* addr_obj(vh){
//	char sym[VLEN+1];
//	canon(&sym);
//	return _addrval(sym,vh->vartab, vh->nxtvar-1);
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
		fun[e].fvar, fun[e].evar, (int)(fun[e].datused) );
}

void dumpFun() {
	fprintf(stderr,"\nfun table: fvar, evar, prused");
	int i;
	int num = curfun-fun;
	for(i=0;i<=num;++i) {
		dumpFunEntry(i);
	}
}

void dumpVar(struct var *v) {
	if(v->type=='A' || v->type=='C') {
		fprintf(stderr,"\n class %s type %c",v->name, v->type);
		if(*(v->vdcd.cd.parent))fprintf(stderr," extends %s", v->vdcd.cd.parent);
	}
	if(v->type=='o') {
		fprintf(stderr,"\n oref: %s type %c classvar %x (%s) blob %x"
				,v->name, v->type,v->vdcd.od.cls, v->vdcd.od.cls->name
				,v->vdcd.od.blob);
	}
	else
		fprintf(stderr,"\n var %p: %s %d %s %d %d "
			, v, (*v).name, (*v).vdcd.vd.class, typeToWord((*v).type)
			, (*v).vdcd.vd.len, (*v).vdcd.vd.value.ui );
}

void dumpVarTab(struct varhdr *vh) {
	int pos = 0;
	fprintf(stderr,"\nVar Table: name class type len (type)value");
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
	fprintf(stderr,"   %9p %9d %9d %9d\n"
			,vh->vartab,vh->nxtvar-vt,vh->gltab-vt,vh->val-(char*)vt);
	fprintf(stderr,"  val          used      endval\n");
	fprintf(stderr,"   %9p %9d %9d\n",vh->val,vh->datused-dt,vh->endval-dt);
	fprintf(stderr,"nxtvar,gltab are decimal sizes in vartab");
	fprintf(stderr,"\nused,endval are decimal sizes in/of val");
}
void dumpBlobTab() {
	struct blob *b;
	fprintf(stderr,"\nvvv blob table vvv\n");
	for(b=blobtab; b<nxtblob; ++b) {
		fprintf(stderr,"\n%s: ",b);
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
void _newblob(char* name, char* blob){
	if(nxtblob >= eblob)eset(TMBLOBERR);
	struct blob *b = nxtblob++;
	strcpy(b->name,name);  //strlen(name) must be <= VLEN
	b->varhdr = blob;
}

#if 0
//Assumes blob is malloc'd and fname,lname defines blob name
void newblob(char* blob){
	if(nxtblob >= eblob)eset(TMBLOBERR);
	struct blob *b = nxtblob++;
	canon(b);
	b->varhdr = blob;
}
#endif

//Assumes sym is canonicalized
struct varhdr* _getblob(char* sym){
	struct blob *b;
	for(b=blobtab; b<nxtblob; ++b) {
    	if( !strcmp(b->name, sym) ) {
    		return b->varhdr;
    	}
  	}
}

/*	A class blob is named the same as the class whose vars it defines.
 *	Globals and locals are named __Globals__ and __Locals__ resp.
 *	Assumes symName() has just parsed and defined fname,lname.
 */
struct varhdr* getblob(){
  char sym[VLEN+1];
  memset(sym,0,VLEN+1);
  canon( &sym );
  return _getblob(sym);
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

/*	Pass one if vartab is NULL computes the needed sizes. Pass two does
 *	the actual link into vartab, which also has room for all values.
 *	The logic here mimics classical void link().
 */
void lnpass12(char *from, char *to, struct varhdr *vh, int newop) {
	char* x;
	char* savedEndapp=endapp;
	char* savedCursor=cursor;
	struct var *vartab;
	if(vh==NULL){
		vartab=NULL;   // pass 1 
		xxpass=1;
	} else {
		vartab=vh->vartab;
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
			char cname[VLEN+1], ename[VLEN+1];
			*cname=*ename=0;
			int abst=0;
			char *where;
			if( *(cursor-1)=='t') {
				abst=1;
				if(_lit(xclass)) ;
				else eset(SYNXERR);
			}
			if(symName()) {     /* class name */
				union stuff kursor;
				kursor.up = cursor = lname+1;
				canon(cname);
				if(_lit(xextends)){
					if(symName()){   // parent name
						cursor=lname+1;
						canon(ename);
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
				cls_dcl(abst,cname,ename,vh,where);
			}
		}
		else if(symName()) {     /* fctn decl */
			union stuff kursor;
			cursor = lname+1;
			_rem();
			kursor.up = cursor;
			newvar('E',2,0,&kursor,vh);
			if( (x=_mustFind(cursor, endapp, '[',LBRCERR)) ) {
				cursor=x+1;
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
	if(verbose[VL])dumpVarTab(vartab);
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
	if(error)return NULL;
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
        blob = vh = mymalloc("blob", size);
        _newblob(blobName,blob);
		memset(vh, 0, size);
        vh->vartab = vh->nxtvar = vh->gltab = vh+1;
        vh->val = vh->datused = vh->vartab + lndata.nvars;
        vh->endval = blob + size;
        f=from; t=to;
		strncpy(par_buf,blobName,VLEN+1);
        while(f){
	        lnpass12(f,t,blob,newop);    // PASS two
	        if(newop){
	        	ascend(par_buf,&f,&t);
	        }
	        else break;
        }
		strncpy(par_buf,blobName,VLEN+1);  // just in case
        cursor=savedcursor;
        endapp=savedendapp;
#if 0
if(newop){
fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
dumpBlob(vh);
dumpVarTab(vh);
}
#endif
        return blob;
}

/* links the loaded program. Uses cursor and endapp globals 
 *	which are set by the loader. Returns pointer to vartab, whose
 *	value references point to space in the same malloc.
 */
void toclink() {
	struct varhdr *vh;	
	vh = lnlink(cursor,endapp,"__Globals__");
}

#if 0
// useful code lines...
if(newop){

fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
dumpBlobTab();
dumpBlob(blob);
}
#endif