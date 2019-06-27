#include "toc.h"

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
void lnpass12(char *from, char *to, struct var *vartab) {
	char* x;
	char* savedCursor=cursor;
	char* savedEndapp=endapp;
/*	check Brackets from cursor to limit*/
	cursor=pr;
	if(checkBrackets(from,to))eset(RBRCERR+1000);
	if(error)whatHappened();
	cursor=from;
	endapp=to;
	newfun();
	while(cursor<endapp && !error){
		char* lastcur = cursor;
		_rem();
		if(_lit(xlb)) _skip('[',']');
		else if( _decl(vartab) ) ;
		else if( _lit(xendlib) ){
			if(curfun==fun) {   /* 1st endlib, assume globals follow */
				newfun();
				curglbl=curfun;
			}
			else {        /* subsequent endlib, 
			                 move assummed globals to frame 0 */
				fun[0].lvar = nxtvar-1;     /* moved */
				fun[1].fvar = nxtvar;      /* globals now empty */
			}
		}
		else if(_symName()) {     /* fctn decl */
			union stuff kursor;
			kursor.up = cursor = lname+1;
			newvar('E',2,1,&kursor,vartab);
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
//printf("~985 %d %d %d %c\n",lpr-pr,apr-pr,cursor-pr,*cursor);
		if(cursor==lastcur)eset(LINKERR);
	}
	cursor = savedCursor;
	if(verbose[VL])dumpVarTab();
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
 *	The real link toclink(char *from, char *to) uses _12link as a service.
 */
void* lnlink(char *from, char *to){
        int size;
        void* blob;
        char* savedcursor=cursor;
        char* savedendapp=endapp;
        cursor=from;
        endapp=to;
        lndata.nvars = lndata.valsize = 0;
printf("toclink~91: from,to %d %d\n",from-pr,to-pr);
        lnpass12(from,to,NULL);
printf("      ~101: nvars,valsize %d %d\n",lndata.nvars,lndata.valsize);
exit(0);
        size = lndata.nvars*sizeof(struct var) + lndata.valsize;
        blob = malloc(size);
        lnpass12(blob,lndata.nvars,blob);
        cursor=savedcursor;
        char* endapp=savedendapp;
        return blob;
}

/* links the loaded program. Uses cursor and endapp globals 
 *	which are set by the loader. Returns pointer to vartab, whose
 *	value references point to space in the same malloc.
 */
void toclink() {
	lnlink(cursor,endapp);
}


