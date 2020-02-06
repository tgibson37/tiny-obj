#include "var.h"
#include "link.h"
#include "toc.h"

int newop;

/*	getters
 */
int getclass(struct var *v){
	if(v->type=='o')return v->vdcd.od.class;
	return v->vdcd.vd.class;
}
int getlen(struct var *v){
	if(v->type=='o')return v->vdcd.od.len;
	return v->vdcd.vd.len;
}
struct varhdr* getvarhdr(struct var *v){
	if(v->type == 'o')return v->vdcd.od.blob;
	else eset(TYPEERR);
	return NULL;
}
char* getvarwhere(struct var *v){
	char *w;
//	union stuff foo;
//	int class = getclass(v);
//	if(class==1){
#if 0
		if(v->type=='o'){
			struct varhdr *vh = getvarhdr(v);
			w = (char*)(vh->vartab->vdcd.od.blob);
		}
		else w = (v->vdcd.vd.value.up);
#endif
	w = (v->vdcd.vd.value.up);

//	}
//	else w=
	return w;
}

int isfcn(struct var *v) {
	int class;
	if(v->type=='o')class = v->vdcd.od.class;
	else class = v->vdcd.vd.class;
	return class=='E';
}

/*	fetch a value and type given its var. On success return len
 *	(or 1 for obj-ref) else 0. Details...
 *		Full retrieval into struct *stuff s for char or int.
 *		Pointer into val space for char* or int*.
 *		Pointer to blob for object ref.
 *	Used only for data, not for type 'E'	
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

/*	SITUATION: Function call parsed PLUS two calls during linking.
 *	Open new var and value frames for library, globals, and 
 *	function locals.
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
}

/*********** var tools ****************/

/*	copy the argument value into the new local place.
 */
int _copyArgValue(struct var *v, int class, Type type, union stuff *passed ) {
	if(passed && class){	 					/* passed pointer */
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
void newvar( int class, Type type, int len, struct var *objclass,
			union stuff *passed, struct varhdr *vh ) {
	int obsize = typeToSize(class,type);
	if(vh==NULL){
		lndata.nvars += 1;
		lndata.valsize += len*obsize;
		return;
	}
	struct var *v = vh->nxtvar;
	struct var *evar = (struct var*)vh->val;
	canon(v);		/* sets the canon'd name into v */
	(*v).type = type;
	if(type=='o'){
		v->vdcd.od.class = class;
		v->vdcd.od.len = len;
		v->vdcd.od.ocl = objclass;
		v->vdcd.od.blob = NULL;	// filled in when known
	}
	else{
		(*v).vdcd.vd.class = class;
		(*v).vdcd.vd.len = len;
		(*v).vdcd.vd.brkpt = 0;
	}
	if(_allocSpace(v,len*obsize,vh)) return;	/* eset done if true */
	if(passed) _copyArgValue( v, class, type, passed);
	if(curfun>=fun) curfun->evar = vh->nxtvar;
	if( vh->nxtvar++ >= evar )eset(TMVRERR);
	if(verbose[VV])dumpVar(v);
	return;
}

/*	Refenence to an object: refname (fname,lname), type 'o', 
 *	details: class entry (cls), blob to referenced object's blob (NULL) 
 *	to be filled in when known.
 */
void newref(struct var *ocls, struct varhdr *vh) {
	struct var *r = vh->nxtvar;
	canon(r);
	r->type = 'o';
	r->vdcd.od.ocl = ocls;
	r->vdcd.od.blob=NULL;
	vh->nxtvar++;
}

/*	Canonicalizes the name bracket by f,l inclusive into buff,
 *	and returns buff. Size of buff must be at least VLEN+1.
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
/*	if *cursor is alphanum find end of sym and canon into buff. 
 *	Set f/lname to matched symbol. 
 */
int canonIf(char* buff){
	char *c = cursor;
	while(isalnum(*c)||*c=='_')++c;
	if(c==cursor)return 0;	// not a symbol
	fname=cursor; lname=c-1;
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
	v = _addrval( sym, curfun->fvar, curfun->evar );	// locals
	if(!v && curobj)																	// instances
			v = _addrval( sym, curobj->vartab, curobj->nxtvar-1);
	if(!v) v = _addrval( sym, curglbl->fvar, curglbl->evar ); //globals
	if(!v) v = _addrval( sym, fun->fvar,fun->evar );	//libs
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
struct var* _isClassName(int nodot) {
	if(!symName())return NULL;
	if(nodot && (*(lname+1)=='.'))return NULL;
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

void dumpFunEntry( int e ) {
	fprintf(stderr,"\n fun entry at %d:	%p %p %p", e,
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

char* classToWord_v(struct var *v){
	return classToWord((*v).vdcd.vd.class);
}

void dumpVar(struct var *v) {
	if(v->type=='A' || v->type=='C') {
		fprintf(stderr,"\n class %s type %c ",v->name, v->type);
		if(*(v->vdcd.cd.parent))fprintf(stderr,"extends %s ", v->vdcd.cd.parent);
	}
	else if(v->type=='o') {
		fprintf(stderr,"\n oref: %s type %c classvar %p (%s) blob %p "
				,v->name, v->type,v->vdcd.od.ocl, v->vdcd.od.ocl->name
				,v->vdcd.od.blob);
	}
	else {
		fprintf(stderr,"\n var %p: %s %s %s len %d %zd %p "
			, v, (*v).name, classToWord_v(v), typeToWord((*v).type)
			, (*v).vdcd.vd.len, (*v).vdcd.vd.value.ui
			, (*v).vdcd.vd.value.up );
	}
}

void dumpVarTab(struct varhdr *vh) {
	if(!vh || !vh->vartab){
		fprintf(stderr,"\nVar Table: not built yet");
fprintf(stderr,"\n  vh %p", vh);
if(vh)fprintf(stderr,"\n  vh->vartab %p  ", vh->vartab);
		return;
	}
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
	fprintf(stderr,"	vartab			 nxtvar		gltab		 evar(val)\n");
	fprintf(stderr,"	 %9p %9zd %9zd %9zd\n",vh->vartab
		,(ptrdiff_t)vh->nxtvar-(ptrdiff_t)vt,(ptrdiff_t)vh->gltab-(ptrdiff_t)vt
		,(ptrdiff_t)vh->val-(ptrdiff_t)vt);
	fprintf(stderr,"	val					used			endval\n");
	fprintf(stderr,"	 %9p %9zd %9zd\n",vh->val
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
	strcpy(b->name,name);	//strlen(name) must be <= VLEN
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
	int s;
	while(from<to) {
		while(*(from++) != '[' && from<to) ;
		if(from<to) {
			s=skip_tool('[',']',from,to);
			if(s<0)return s;	 //bad
		}
	}
	return 0;	 //good
}

#if 0					 // useful code lines...
dumpft(fname,lname);
fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
dumpBlobTab();
dumpBlob(blob);
if(newop){
}
#endif
