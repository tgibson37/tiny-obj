#include "link.h"
#include "var.h"
#include "toc.h"

int newop;
int xxpass=0;

/*	pass 2 action for xclass, xabstract */
void cls_dcl(int abst,char *cname,char *ename,struct varhdr *vh, 
			char* where){
	struct var *c = vh->nxtvar++;
	strcpy(c->name,cname);
	c->type = abst?'A':'C';
	strncpy(c->vdcd.cd.parent,ename,VLEN);
	c->vdcd.cd.where = where;
}

/*	add 'this' variable to an object's vartab */
void newvar_this(struct varhdr *vh){
//ps("link~19, newvar_this ");pn(vh);pl("");
//	union stuff stf;
//	stf.up = vh;
	struct var *v = vh->nxtvar;
	struct var *evar = (struct var*)vh->val;
	strcpy(v->name, "this" );
	v->type = 'o';
	v->vdcd.od.class =  0;
	v->vdcd.od.len =  1;
//	v->vdcd.od.ocl = ???;
	v->vdcd.od.blob =  vh;
	if( vh->nxtvar++ >= evar )eset(TMVRERR);
	if(verbose[VV])dumpVar(v);
}

/*	Pass one if varhdr is NULL computes the needed sizes. Pass two does
 *	the actual link into varhdr, which also has room for all values.
 *	The logic here mimics classical void link().
 */
void lnpass12(char *from, char *to, 
			struct varhdr *vh, int newop, char *conName) {
	char* cptr;
	char* savedEndapp=endapp;
	char* savedCursor=cursor;
	if(vh==NULL){
		xxpass=1;
	} else {
		xxpass=2;
		if(!newop)newfun(vh);
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
		else if(lit(xclass)||lit(xabstract)){
			struct var cname, ename;
			int abst=0;
			char *where;
			if( *(cursor-1)=='t') {
				abst=1;
				if(lit(xclass)) ;
				else eset(SYNXERR);
			}
			if(symName()) {     /* class name */
//				union stuff kursor;
//				kursor.up = 
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
				cls_dcl(abst,cname.name,ename.name,vh,where);
			}
		}
#if 0
		if(newop) {
			if(_isClassName(NODOT)){  // obj ref or constructor
//				char *conname, *symname;
//#if 0
fprintf(stderr,"~114 <<<<<<<===================conName=%s==\n",conName);
pft(cursor,cursor+20);
fprintf(stderr,"~118 <<<<<<<===================conName=%s==isRef=%d\n"
	,sym.name,isRef);
}
//#endif
				struct var sym;
				canon(&sym);
				int isRef = strcmp(conName,sym.name);   //Not the constructor
				if(isRef){
					if(xxpass==1){
						lndata.nvars += 1;
						lndata.valsize += sizeof(void*);   // ISSUE: array???
					}
					else if(xxpass==2){ // ISSUE: use varalloc, toc~142 ???
						if(symName()) {		// Game g;
							cursor = lname+1;
							struct var *v = vh->nxtvar;
							v->type = 'o';
							v->vdcd.od.class = 0;
							v->vdcd.od.len = 1;
							allocSpace(v, sizeof(void*), vh);
						}
					}
				}
				else {
//fprintf(stderr,"NEED skip code link~138\n");
//exit(1);
					if((cptr=_mustFind(cursor,endapp,'[',LBRCERR))) {
						cursor=cptr+1;
						skip('[',']');
++cursor;
					}
				}
			}
			else eset(SYMERR);
		}
#endif
		else if(symName()) {     /* fctn decl */
			union stuff kursor;
			cursor = lname+1;
			rem();
			kursor.up = cursor;
			newvar('E',2,0,NULL,&kursor,vh);
			if( (cptr=_mustFind(cursor, endapp, '[',LBRCERR)) ) {
				cursor=cptr+1;
				skip('[',']');
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
//fprintf(stderr,"\n~240: blobname=%s",blobName);
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
        return vh;
}

/* links the loaded program. Uses cursor and endapp globals 
 *	which are set by the loader. 
 */
void toclink() {
	lnlink(cursor,endapp,"__Globals__",NULL);
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
//dumpVarTab(vh);
//fprintf(stderr,"\n~319 error=%d",error);
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
