#include "var.h"
<<<<<<< HEAD
#include "tc.h"
=======
#include "link.h"
#include "toc.h"
>>>>>>> newbase

/* SITUATION: Function call parsed. 
	Open new var and value frames for the functions locals.
 */
void newfun() {
//fprintf(stderr,"\nvar~7: curfun,efun %d %d",curfun-fun,efun-fun);
	if(++curfun>efun){
		eset(TMFUERR);
	} 
	else {
		(*curfun).fvar = nxtvar;
		(*curfun).lvar = nxtvar-1;
		(*curfun).prused = prused;
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
	nxtvar=(*curfun).fvar;
	prused=(*curfun).prused;
	--curfun;
	if(verbose[VV]){
		fflush(stdout);
		fprintf(stderr,"\nfundone %s",fcnName);
	}
}

/*********** var tools ****************/

/*	copy the v's value into *passed.
 */
int _copyArgValue(struct var *v, int class, Type type, union stuff *passed ) {
	if(passed && class){   					/* passed pointer */
		(*v).value.up = (*passed).up;
	} else if( passed && !class ) {			/* passed datum */
		switch(type){
		case Int:
			put_int( (*v).value.up, (*passed).ui );
			break;
		case Char:
			put_char( (*v).value.up, (*passed).uc );
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
int _allocSpace(struct var *v, int amount){
	char* kf;
	kf = prused+1;
	(*v).value.up = prused+1;
	prused += amount;
	if( prused-EPR >=0 ) {
		eset(TMVLERR);
		return TMVLERR;
	}
	memset( kf, 0, prused-kf+1 ); /* zero the reserved space */
	return 0;
}

/* SITUATION: Declaration is parsed, and its descriptive data known.
 *  Fill in the var with this data. Allocate value storage unless already
 *  allocated, i.e. pointer to passed data. Copy passed data into allocation.
 *  NOTE: signifantly refactored.
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
  canon(v);    /* sets the canon'd name into v */
  (*v).type = type;
  if(type=='o'){
    v->vdcd.od.class = class;
    v->vdcd.od.len = len;
    v->vdcd.od.ocl = objclass;
    v->vdcd.od.blob = NULL;  // filled in when known
  }
  else{
    (*v).vdcd.vd.class = class;
    (*v).vdcd.vd.len = len;
    (*v).vdcd.vd.brkpt = 0;
  }
  if(_allocSpace(v,len*obsize,vh)) return;  /* eset done if true */
  if(passed) _copyArgValue( v, class, type, passed);
  if(curfun>=fun) curfun->evar = vh->nxtvar;
  if( vh->nxtvar++ >= evar )eset(TMVRERR);
  if(verbose[VV])dumpVar(v);
  return;
}
#if 0
/* SITUATION: Declaration is parsed, and its descriptive data known.
 * 	Fill in the var with this data. Allocate value storage unless already
 *	allocated, i.e. pointer to passed data. Copy passed data into allocation.
 *	NOTE: signifantly refactored.
 */
void newvar( int class, Type type, int len, union stuff *passed ) {
	struct var *v = &vartab[nxtvar];
	canon(v);    /* sets the canon'd name into v */
	(*v).class = class;
	(*v).type = type;
	(*v).len = len;
	(*v).brkpt = 0;
	int obsize = typeToSize(class,type);
	if(_allocSpace(v,len*obsize)) return;  /* true is bad, eset done */
	if(passed) _copyArgValue( v, class, type, passed);
	if(curfun>=fun) (*curfun).lvar = nxtvar;
	if( ++nxtvar>vtablen )eset(TMVRERR);
	if(verbose[VV])dumpVar(v);
	return;
}
<<<<<<< HEAD
#endif
=======

#if 0
void cls_dcl(int abst,char *cname,char *ename,struct varhdr *vh, char* where){
//dumpBlob(vh);
	struct var *c = vh->nxtvar++;
	strcpy(c->name,cname);
	c->type = abst?'A':'C';
	strncpy(c->vdcd.cd.parent,ename,VLEN);
	c->vdcd.cd.where = where;
//dumpVar(c);
}
#endif

/*  Refenence to an object: refname (fname,lname), type 'o', 
 *  details: class entry (cls), blob to referenced object's blob (NULL) 
 *  to be filled in when known.
 */
void newref(struct var *ocls, struct varhdr *vh) {
  struct var *r = vh->nxtvar;
  canon(r);
  r->type = 'o';
  r->vdcd.od.ocl = ocls;
  r->vdcd.od.blob=NULL;
  vh->nxtvar++;
}

>>>>>>> newbase
/* Canonicalizes the name bracket by f,l inclusive into buff, and returns buff.
	sizeOf buff must be at least VLEN+1.
 */

char* canon_buff(char* first, char* l, char* buff) {
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
	canon_buff(fname,lname,(*v).name);
}
#if 0
	char* n=(*v).name;
	while( n < ((*v).name)+VLEN ) *n++ = 0;
	int len = lname-fname+1;
	len = len>VLEN ? VLEN : len;
	/* zap name field of v */
	strncpy( (*v).name, fname, len );  /* pads with nulls if short */
	(*v).name[8] = 0;     /* so long name canonicalized as a string */
	int length = lname-fname+1;
	if(length>VLEN) {
		(*v).name[VLEN-1] = *lname; 
	} 
#endif


/* 	looks up a symbol at one level
 */
struct var* _addrval(char *sym, struct funentry *level) {
	int first = (*level).fvar;
	int last  = (*level).lvar;
	int pvar;
	for(pvar=first; pvar<=last; ++pvar) {
		if( !strcmp(vartab[pvar].name, sym) ) {
			if( debug && (vartab[pvar]).brkpt==1 )br_hit(&vartab[pvar]);
			return &vartab[pvar];
		}
	}
	return 0;
}

/* 	looks up a symbol at three levels
 */
struct var* addrval_all(char *sym) {
	struct var *v;
	v = _addrval( sym, curfun );
	if(!v) v = _addrval( sym, curglbl );
	if(!v) v = _addrval( sym, fun ); 
	if(v)return v;
	return 0;	
}

/* 	looks up a symbol pointed to by fname,lname: 
 *	locals, globals, library levels in that order. First hit wins.
 */
struct var* addrval() {
	struct var sym;
	canon( &sym );
	struct var *av = addrval_all(sym.name);
	return av;
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
	fprintf(stderr,"\n fun entry at %d:  %d %d %d", e,
		fun[e].fvar, fun[e].lvar, (int)(fun[e].prused-pr) );
}

void dumpFun() {
	fprintf(stderr,"\nfun table: fvar, lvar, prused");
	int i;
	int num = curfun-fun;
	for(i=0;i<=num;++i) {
		dumpFunEntry(i);
	}
}

void dumpVar(struct var *v) {
//fprintf(stderr,"\n~200V");
	fprintf(stderr,"\n var %d: %s %d %s %d ", (int)(v-vartab),
		(*v).name, (*v).class, typeToWord((*v).type), (*v).len );
/*	if((*v).value.up) 
		fprintf(stderr," ref to pr[%d]", (char*)((*v).value.up)-pr);
*/
		dumpVal( (*v).type, (*v).class, &((*v).value), 0 );
}

void dumpVarTab() {
	int pos = 0;
	fprintf(stderr,"\nVar Table: name class type len (type)value");
	struct var *v = vartab-1;
	while(++v < &vartab[nxtvar]) {
//fprintf(stderr,"\n~213V");
		dumpVar(v);
		++pos;
	};
	if( !pos )fprintf(stderr," empty");
}
<<<<<<< HEAD
=======

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
  int s;
  while(from<to) {
    while(*(from++) != '[' && from<to) ;
    if(from<to) {
    	s=skip_tool('[',']',from,to);
    	if(s<0)return s;   //bad
    }
  }
  return 0;   //good
}


#if 0
// useful code lines...
dumpft(fname,lname);
fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
dumpBlobTab();
dumpBlob(blob);
if(newop){
}
#endif
>>>>>>> newbase
