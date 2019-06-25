#include "toc.h"
#include "link.h"
#include "fileread.h"
#include "parse.h"

/*	Processes a declaration using (*f) to do the actual work. f is getdata for pass 1
 *	and varalloc for pass 2.
 */

struct data { int nvars; int valsize; } lndata;

char* _canon(char* first, char* l, char* buff) {
        int i=0; 
        char* f=first;
        while(f<=l && i<VLEN-1) buff[i++]=*(f++);
        if(f<=l)buff[i++]=*l;
        buff[i]=0;
        return buff;
}

void getvardata(Type t,int zero) {
	++ lndata.nvars;
}
int isdecl( void (*f)(Type,int) ) { 
	if( lit(xchar) ) {
		do {
			(*f)( Char, 0 );
		} while( lit(xcomma) );
	} else if( lit(xint) ) {
		do {
			(*f)( Int, 0 );
		} while( lit(xcomma) );
	} else {
		return 0;  /* not decl */
	}
	lit(xsemi);    /* is decl */
	return 1;
}

void getclassdata() {
	++ lndata.nvars;
}

int isclass( void (*f)() ) {
	int abs=0,ext=0;
	char classname[VLEN+1];
	char basename[VLEN+1];

	if( lit(xabstract) ){
		abs=1;
	}
	if( lit(xclass) ){
		(*f)();
		if(symname()){
			cursor=lname+1;
			_canon(fname,lname,classname);
		}
		else eset(SYMERR);
		if( lit(xextends) ){
			ext=1;
			if(symname()){
				cursor=lname+1;
				_canon(fname,lname,basename);
			}
			else eset(SYMERR);  // symbol is required
		}
		else {
			int x=mustfind(cursor,endapp, '[', LBRCERR);
			if(x){
				cursor = x+1;
				skip('[',']');
			}
			else eset(LBRCERR);
		}
		printf("\nlink~64, class: %s: %d%d ",classname,abs,ext);
		if(ext)ps(basename);
		return 1;
	}
	else return 0;
//dumpcurl("~66");
}

void getsymdata() {
	++lndata.nvars;
}
int issym( void (*f)() ){
	int x;
pl("in issym: ");pft(fname,lname);
dumpcurl("~77");				
	if(symname()) {
		cursor=lname+1;
		(*f)();
		if( (x=mustfind(cursor,endapp, '[', LBRCERR)) ) {
			cursor=x+1;
			skip('[',']');
			return 1;
		}
		else return 0;
	}
}

/*	scans from from to to-1 counting vartab entries, and value space size
 */
void lngetdata(){
	char *lastcur;
	char *x;
	lndata.nvars = 0;
	lndata.valsize = 0;
	rem();
	while(cursor<endapp && !error){
		lastcur=cursor;
		if(lit(xlb)) {
			skip('[',']');
		}
		else if( isdecl( getvardata ) ){}
		else if( isclass( getclassdata ) ){}
		else if( issym( getsymdata ) ) {}     /* fctn decl */
		else if(*cursor=='#'){
			while( ++cursor < endapp ) {
				if( (*cursor==0x0d)||(*cursor=='\n') )break;
			}
		}
		if(cursor==lastcur){
			eset(LINKERR); 
			break;
		}
		rem();
	}
}

/*	scans from from to to-1 building vartab
 */
int lndolink(struct var *vartab, int tabsize){
printf("\nlink~123: nvars,valsize %d %d", lndata.nvars, lndata.valsize);
}

/*	allocates space for vartab and values, builds vartab
 */
void* link(char *from, char *to){
	int size;
	void* blob;
	char* savedcursor=cursor;
	char* savedendapp=endapp;
	cursor=from;
	endapp=to;
printf("link~141: from,to %d %d\n",from-pr,to-pr);
	lngetdata();
	size = lndata.nvars*sizeof(struct var) + lndata.valsize;
	blob = malloc(size);
	lndolink(blob,lndata.nvars);
	cursor=savedcursor;
	char* endapp=savedendapp;
}
/*	test this mess!
 */
int main(int argc, char **argv) {
	int len;
	len = fileRead("SamplePrograms/first.toc",pr,999);
	lndata.nvars = 0;
	lndata.valsize = 0;
	cursor=pr;
	link(pr,pr+len);
	printf("\ndone\n");
	return 0;
} 

