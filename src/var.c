#include "toc.h"

/* SITUATION: Function call parsed. 
	Open new var and value frames for the functions locals.
 */
void newfun() {
	if(++curfun>efun){
		eset(TMFUERR);
	} 
	else {
		curfun->fvar = curfun->evar = locals->nxtvar;
		curfun->datused = locals->datused;
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
int _allocSpace(struct var *v, int amount, struct varhdr *vh){
	if( vh->datused+amount > EPR ) {
		eset(TMVLERR);
		return TMVLERR;
	}
	v->value.up = vh->datused;
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
//printf("var~105 nxtvar len %d %d\n",nxtvar,nxtvar-vh);
	struct var *v = vh->nxtvar;
	canon(v);    /* sets the canon'd name into v */
	(*v).class = class;
	(*v).type = type;
	(*v).len = len;
	(*v).brkpt = 0;
	if(_allocSpace(v,len*obsize,vh)) return;  /* true is bad, eset done */
	if(passed) _copyArgValue( v, class, type, passed);
	if(curfun>=fun) curfun->evar = vh->nxtvar;
	if( vh->nxtvar++ >= vh->val )eset(TMVRERR);
fprintf(stderr,"~122 nxtvar,evar,error %p %p %d    ",vh->nxtvar,vh->val,error);
	if(verbose[VV])dumpVar(v);
	return;
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
//printf("~129 %p <- %s ",buff,buff);
	return buff;
}

/* 	fname..lname is full name. Puts canonicalized name into v. If short
 *	enough same as name. If longer first VLEN-1 characters + last character.
 *	The last char has more info than middle chars.
 */
void canon(struct var *v) {
	_canon(fname,lname,(*v).name);
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


/* 	looks up a symbol
 */
struct var* _addrval(char *sym, struct var *first, struct var *last) {
	struct var *pvar;
	for(pvar=first; pvar<=last; ++pvar) {
		if( !strcmp(pvar->name, sym) ) {
			if( debug && (pvar->brkpt==1) )br_hit(pvar);
			return pvar;
		}
	}
	return 0;
}

/* 	looks up a symbol at three levels via function table
 */
struct var* addrval_all(char *sym) {
	struct var *v;
	v = _addrval( sym, curfun->fvar, curfun->evar );
	if(!v) v = _addrval( sym, curglbl->fvar, curglbl->evar );
	if(!v) v = _addrval( sym, fun->fvar,fun->evar ); 
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
	fprintf(stderr,"\n var %p: %s %d %s %d ", v,
		(*v).name, (*v).class, typeToWord((*v).type), (*v).len );
}

void dumpVarTab(struct varhdr *vh) {
	int pos = 0;
	fprintf(stderr,"\nVar Table: name class type len (type)value");
	struct var *v = vh->vartab;
	while(v < vh->val) {
		dumpVar(v++);
		++pos;
	};
	if( !pos )fprintf(stderr,"\nempty\n");
	else fprintf(stderr,"\n");
}

void dumpBlob(struct varhdr *vh){
	fprintf(stderr,"Blob (aka varhdr) at %p \n",vh );
	struct var *vt = vh->vartab;
	char* dt = vh->val;
	fprintf(stderr,"  vartab       nxtvar    gltab     evar(val)\n");
	fprintf(stderr,"   %9p %9d %9d %9d\n"
			,vh->vartab,vh->nxtvar-vt,vh->gltab-vt,vh->val-(char*)vt);
	fprintf(stderr,"  val          used      endval\n");
	fprintf(stderr,"   %9p %9d %9d\n",vh->val,vh->datused-dt,vh->endval-dt);
	fprintf(stderr,"nxtvar,gltab,endval,used are decimal relative to vartab,val\n");
}

void dumpBV(struct varhdr *vh){ 
	dumpBlob(vh); 
	dumpVarTab(vh->vartab);
}

// Enters blob into blobtab. 
void _newblob(char* name, char* blob){
	if(nxtblob >= eblob)eset(TMBLOBERR);
	struct blob *b = nxtblob++;
	strcpy(b->name,name);  //strlen(name) must be <= VLEN
	b->varhdr = blob;
}

//Assumes blob is malloc'd and fname,lname defined blob name
void newblob(char* blob){
	if(nxtblob >= eblob)eset(TMBLOBERR);
	struct blob *b = nxtblob++;
	canon(b);
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
}
//Assumes symname() has just parsed and defined fname,lname
struct varhdr* getblob(){
  char sym[VLEN+1];
  memset(sym,0,VLEN+1);
  canon( &sym );
  return getblob(sym);
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

/*	Pass one if vartab is NULL computes the needed sizes. Pass two does
 *	the actual link into vartab, which also has room for all values.
 *	The logic here mimics classical void link().
 */
int xxpass=0;
void lnpass12(char *from, char *to, struct varhdr *varhdr) {
	char* x;
	char* savedEndapp=endapp;
	char* savedCursor=cursor;
	struct var *vartab;
	if(varhdr==NULL)vartab=NULL;   // pass 1 
	else {
fprintf(stderr,"~%s %d:\n",__FILE__,__LINE__);
dumpBlob(varhdr);
		newfun();
	}
/*	check Brackets from cursor to limit*/
	cursor=pr;
	if(checkBrackets(from,to))eset(RBRCERR+1000);
	if(error){ whatHappened(); exit(1); }
	cursor=from;
	endapp=to;
	while(cursor<endapp && !error){
		char* lastcur = cursor;
		_rem();
		if(_lit(xlb)) _skip('[',']');
		else if( _decl(vartab) ) ;
		else if( _lit(xendlib) ){
			if(varhdr != NULL){     //  <<==  PASS TWO endlibrary
				if(curfun==fun) {   /* 1st endlib, assume app globals follow */
					newfun();
					curglbl=curfun;
				}
				else {        // multiple endlib tolerance, undo the assumption
					fun[0].evar = fun[1].fvar = varhdr->nxtvar;
				}
				varhdr->gltab = varhdr->nxtvar;
			}
		}
		else if(_symName()) {     /* fctn decl */
			union stuff kursor;
			kursor.up = cursor = lname+1;
			newvar('E',2,1,&kursor,vartab);
			if(varhdr && (!error))varhdr->nxtvar++;   //keep sync'd with global nxtvar
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
		if(cursor==lastcur)eset(LINKERR);
	}
	cursor = savedCursor;
	if(verbose[VL])dumpVarTab(vartab);
}

/*	build its vartab, make entry into owner's vartab
 */
void classlink(struct var *vartab){
	return;   // model for this in Resources link.c/h
}

/*	Modified version of tclink() to parse and build class tables. 
 *	Links source text area from *from to *to in two passes. In pass one 
 *	vartab is NULL and computes the size of memory needed for both the 
 *	vartab and the value space. In pass two it builds the table. 
 *	Parse, var, and other services are supplied by whole unchanged tiny-c files.
 *	Uses lnpass12 as a service.
 */
void* lnlink(char *from, char *to){
        int size;
        void* blob;
        struct varhdr *vh;
        char* savedcursor=cursor;
        char* savedendapp=endapp;
        cursor=from;
        endapp=to;
        lndata.nvars = lndata.valsize = 0;
        lnpass12(from,to,NULL);    // PASS one
fprintf(stderr,"~%s %d:\n",__FILE__,__LINE__);
fprintf(stderr,"  ~ +1: lndata: %d %d",lndata.nvars,lndata.valsize);
        size = sizeof(struct varhdr) + lndata.nvars*sizeof(struct var) + lndata.valsize;
        blob = vh = malloc(size);
        _newblob("_Globals",blob);
		memset(vh, 0, size);
        vh->vartab = vh->nxtvar = blob + sizeof(struct varhdr);
        vh->datused = vh->val = vh->vartab + lndata.nvars*sizeof(struct var);
        vh->endval = blob + size;
		cursor=from;
fprintf(stderr,"\npass ONE done, sizeof var %d\n\n",sizeof(struct var));
dumpBlob(blob);
        lnpass12(from,to,blob);    // PASS two
fprintf(stderr,"\npass TWO done\n\n");
dumpBlob(blob);
        cursor=savedcursor;
        endapp=savedendapp;
        return blob;
}

/* links the loaded program. Uses cursor and endapp globals 
 *	which are set by the loader. Returns pointer to vartab, whose
 *	value references point to space in the same malloc.
 */
void toclink() {
	struct varhdr *vh;	
	vh = lnlink(cursor,endapp);
}
