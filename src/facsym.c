#include "factor.h"
#include "expr.h"
#include "stack.h"
#include "var.h"

/*	dot is parsed */
struct var* obsym(struct varhdr *qvh){
	if(symName()){
		struct var *ovar;
		if(!qvh){
			eset(SYMERR);
			return NULL;
		}
		ovar = addr_obj(qvh);
		cursor = lname+1;
		if(!ovar)eset(SYMERR);
		return ovar;
	}
	else eset(SYNXERR);
	return NULL;
}

/*	var *v is an object. Parse its subscript if v is an array, and
 *	return its blob. Decrement class if subscripted.
 */
struct varhdr **getvhcell(struct var *v){
	int len = v->vdcd.od.len;
	int class = v->vdcd.od.class;
	struct varhdr **blobarray = v->vdcd.od.blob;
	int subscript=0;
	if( class>0 ){
		if(lit(xlpar)){
			asgn(); 
			if(error)return NULL;
			lit(xrpar);
			subscript = toptoi();
			if( subscript<0 || subscript>=len ){eset(RANGERR);return NULL;}
		}
	}
	
	return blobarray+subscript;
}

/*  Situation: A symbol is parsed, f/lname defined.
 *  Action: parse the whole factor gathering information,
 *  and if fcn _enter else push onto stack.
 */
union stuff facsym_value;
void facsym() {
	struct var *v = addrval();
	if (!v) {eset(SYMERR);return;}
	if (v->type=='o') {
		struct varhdr **cell = getvhcell(v);
		// type 'o' sym[(N)] parsed. class,v,vh defined
		if (*(cursor)== '.') {
			++cursor;
			struct varhdr *vh = *cell;
			v = obsym(vh);
			canobj = vh;
		} else { 				// obj ref
            facsym_value.up = cell;
            pushst(0,'L','o',&facsym_value);
            return;
		}
	}
	if(!v){eset(SYMERR);return;}
	if(isfcn(v))_enter(getvarwhere(v));
	else _pushvar(v); //  parses subscript if present
}

//fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
