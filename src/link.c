#include "link.h"
#include "var.h"
#include "expr.h"
#include "toc.h"
#include "factor.h"

extern int dump_mallocs;
int newop;
int xxpass=0;

/*	pass zero actually done during pass one to assure class names
 *	are recognized. A simple substitute for _addrval(..). Dimension
 *	is limit on number of class definitions.
 */
char *passZero[100];
int nxtZero;

/*	add 'this' variable to an object's vartab */
void newvar_this(struct varhdr *vh){
	struct var *v = vh->nxtvar;
	struct var *evar = (struct var*)vh->val;
	strcpy(v->name, "this" );
	v->type = 'o';
	v->vdcd.od.class =  0;
	v->vdcd.od.len =  1;
//	v->vdcd.od.ocl = ???;
	v->vdcd.od.blob = (struct varhdr**)vh;
	if( vh->nxtvar++ >= evar )eset(TMVRERR);
	if((verbose[VV])&&(!verbose_silence))dumpVar(v);
}

void cls_define(int abst,char *cname,char *ename,struct varhdr *vh, 
			char* where){
	struct var *c = vh->nxtvar++;
	strcpy(c->name,cname);
	c->type = abst?'A':'C';
	strncpy(c->vdcd.cd.parent,ename,VLEN);
	c->vdcd.cd.where = where;
}

// class or abstract parsed
void cls_action(struct varhdr *vh){
	struct var cname, ename;
	int abst=0;
	char *where;
	if( *(cursor-1)=='t') {
		abst=1;
		if(lit(xclass)) ;
		else eset(SYNXERR);
	}
	if(symName()) {     /* class name */
		if(xxpass==1){
			passZero[nxtZero++]=fname;
			if(nxtZero>100)eset(TMCLASSERR);
		}
		cursor = lname+1;
		canon(&cname);
		if(lit(xextends)){
			if(symName()){   // parent name
				cursor=lname+1;
				canon(&ename);
			}
			else eset(SYNXERR);
		}
		rem();
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
		cls_define(abst,cname.name,ename.name,vh,where);
	}
}

int bar=0;
// class name NODOT parsed, not constructor
void objdecl_action(struct var *isvar, struct varhdr *vh){
	if(symName()) {		// Game g;
		do{
			varalloc('o',isvar,NULL,vh);	
		} while(lit(xcomma));
	}
}

// class name NODOT parsed, IS constructor
void constr_def(struct varhdr *vh){
	char *cptr;
	union stuff kursor;
	cursor = lname+1;
	rem();
	kursor.up = cursor;
	newvar('E',2,0,NULL,&kursor,vh);
	if((cptr=_mustFind(cursor,endapp,'[',LBRCERR))) {
		cursor=cptr+1;
		skip('[',']');
	}
}

// class name parsed, two cases: constructor and object declaration
void class_name(struct var *isvar, struct varhdr *vh, char *conName){
	struct var sym;
	canon(&sym);
	int isRef = strcmp(conName,sym.name);
	if(isRef){
		objdecl_action(isvar,vh);
	}
	else {
		constr_def(vh);			
	}
}

// decl of global var of type 'o'
void global_object_decl(struct var *isvar, struct varhdr *vh){
	do {
		if(symName())varalloc( 'o', isvar, NULL, vh );
		else eset(SYMERR);
	} while( lit(xcomma) );
}

// obj dcl case: test and action, return TRUE if done.
int isObjDcl(struct var *isvar, struct varhdr *vh){
	for(int i=0;i<nxtZero;++i){
		if(!strncmp(fname,passZero[i],lname-fname+1)){
			objdecl_action(isvar,vh);
			return 1;
		}
	}
	return 0;
}
// symbol parsed, two cases: function definition, object decl
void sym_action(struct var *isvar, struct varhdr *vh){
	if(isObjDcl(isvar,vh))return;
	union stuff kursor;
	char *cptr;
	cursor = lname+1;
	rem();
	kursor.up = cursor;
	newvar('E',2,0,NULL,&kursor,vh);
	if( (cptr=_mustFind(cursor, endapp, '[',LBRCERR)) ) {
		cursor=cptr+1;
		skip('[',']');
		}
}

void hash_action(struct varhdr *vh){
	while(++cursor<endapp) {
		if( (*cursor==0x0d)||(*cursor=='\n') )break;
	}
}

/*	Pass one if varhdr is NULL computes the needed sizes. Pass two does
 *	the actual link into varhdr, which also has room for all values.
 *	The logic here mimics classical void link().
 */
void lnpass12(char *from, char *to, 
			struct varhdr *vh, int newop, char *conName) {
	struct var* isvar;
	char* savedEndapp=endapp;
	char* savedCursor=cursor;
	if(vh==NULL){
		xxpass=1;
		nxtZero=0;
	} else {
		xxpass=2;
		if(!newop)openVarFrame(vh);    //open var frame in vh
	}
	if(checkBrackets(from,to))eset(RBRCERR+1000);
	if(error){ whatHappened(); exit(1); }
	cursor=from;
	endapp=to;
	while(cursor<endapp && !error){
		char* lastcur = cursor;
		rem();
		if(lit(xlb)) skip('[',']');
		else if( decl(vh) ) ;
		else if( lit(xendlib) ){
			if(vh != NULL){     //  <<==  PASS TWO endlibrary
				if(curfun==fun) {   /* 1st endlib, assume app globals follow */
					if(!newop){
						openVarFrame(vh);
						curglbl=curfun;
					}
				}
				else {        // multiple endlib tolerance, undo the assumption
					fun[0].evar = fun[1].fvar = vh->nxtvar;
				}
				vh->gltab = vh->nxtvar;  // start (or start over) on fun[1]
			}
		}
		else if(lit(xclass)||lit(xabstract)){
			cls_action(vh);
		}
		else if(newop && (isvar=_isClassName(NODOT))) {
			class_name(isvar,vh,conName);
		}
		else if(!newop && (isvar=_isClassName(NODOT))) {
			global_object_decl(isvar,vh);
		}
		else if(symName()) {     /* fctn or obj decl */
			sym_action(NULL,vh);   //  <<==  need vhdr for pass 2
		}
		else if(*cursor=='#'){
			hash_action(vh);
		}
		if(cursor==lastcur){
			eset(LINKERR);
		}
	}
	cursor = savedCursor;
	endapp = savedEndapp;
	if(verbose[VL]&&(xxpass==2)&&(!verbose_silence))dumpVarTab(vh);
}

/*	tools for accessing specific data in a *var. A var can be
 *	id'd two ways: ptr to the var entry (struct var *v), and
 *	symbol name used to install that entry. These use the former.
 *	The latter requires	a search, and that search needs to know 
 *	what vartab to search. visclass(..) searches globals for class name.
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
	if(skip('[',']'))eset(RBRCERR);
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
 *	Parse, var, and other services are supplied by whole unchanged 
 *	tiny-c files.
 *	Uses lnpass12 as a service.
 */
struct varhdr* lnlink(char *from, char *to, 
                       char *blobName, struct var *isclvar){
		if((verbose[VL])&&(!verbose_silence))
			fprintf(stderr,"\nlinking %s",blobName);
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
	        lnpass12(f,t,NULL,newop,blobName);    // PASS one
	        if(newop){
	        	ascend(par_buf,&f,&t);
	        }
	        else break;
        }
        if(newop){
        	lndata.nvars++;   // for 'this'
        	lndata.valsize += sizeof(void*);

        }
        size = sizeof(struct varhdr) 
        		+ lndata.nvars*sizeof(struct var) + lndata.valsize;
        blob = mymalloc(blobName, size+10);
        vh = (struct varhdr*)blob;
        vh->sernum = nxtblob-blobtab+1;
        if(dump_mallocs)fprintf(stderr," sernum %d",vh->sernum);
        _newblob(blobName,blob);
		memset(vh, 0, size);
        vh->vartab = vh->nxtvar = vh->gltab = (struct var*)(vh+1);
        vh->val = vh->datused = (char*)(vh->vartab+lndata.nvars);
        vh->endval = blob + size;
		if(newop)newvar_this(vh);
        f=from; t=to;
		strncpy(par_buf,blobName,VLEN+1);
        while(f){
	        lnpass12(f,t,vh,newop,blobName);    // PASS two
	        if(newop){
	        	ascend(par_buf,&f,&t);
	        }
	        else break;
        }
		strncpy(par_buf,blobName,VLEN+1);  // just in case
        cursor=savedcursor;
        endapp=savedendapp;
        newop=0;
        return vh;
}

/* links the loaded program. Uses cursor and endapp globals 
 *	which are set by the loader. 
 */
void toclink() {
//	struct varhdr *glbls;
//	glbls = 
	lnlink(cursor,endapp,"__Globals__",NULL);
//dumpVarTab(glbls);
}

/* links a class given its *var.
 */
struct varhdr* classlink(struct var *isclvar){
// scope body of cn
	char *from, *to;
	from = isclvar->vdcd.cd.where+1;
	char *temp = cursor;
	cursor=from;
	if(skip('[',']'))eset(LBRCERR);
	to = cursor-1;
	cursor=temp;
// link the body, return blob address
	char *save_fn = fname;  // need class name later
	char *save_ln = lname;
	struct varhdr *vh;
	vh = lnlink(from, to, isclvar->name, isclvar);
	if(error){
		whatHappened();
		exit(1);
	}
	fname = save_fn;
	lname = save_ln;
	return vh;
}

#if 0        // useful code lines...
dumpft(fname,lname);
fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
dumpBlobTab();
dumpBlob(blob);
if(newop){
}
#endif
