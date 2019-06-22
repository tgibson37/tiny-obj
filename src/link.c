#include "toc.h"
#include "link.h"
#include "fileread.h"
#include "parse.h"

char pr[999];
struct data { int nvars; int valsize; } lndata;

/*	Proccesses a declaration using f to do the actual work. f is getdata for pass 1
 *	and varalloc for pass 2. The second argument must be 0 for linking.
 */

void getdecldata(Type t,int zero) {
	++ lndata.nvars;
}

int decl( void (*f)(Type,int) ) { 
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

/*	scans from from to to-1 counting vartab entries, and value space size
 */
int lngetdata(char *from, char *to){
	char *x;
	lndata.nvars = 0;
	lndata.valsize = 0;
	while(from<to && !error){
		rem();
		if(lit(xlb)) {
			skip('[',']');
		}
		else if( decl( getdecldata ) ){}
		else if(symName()) {     /* fctn decl */
			++ lndata.nvars;
			if( (x=mustFind(from, to, '[',LBRCERR)) ) {
				from=x+1;
				skip('[',']');
			}
		}
		else if(*from=='#'){
			while(++from<to) {
				if( (*from==0x0d)||(*from=='\n') )break;
			}
		}
	}
}
/*	scans from from to to-1 building vartab
 */
int lndolink(char *from, char *to, struct var *vartab, int tabsize){
	printf("%d %d\n", lndata.nvars, lndata.valsize);
#if defined(_WIN32)
	int cursor = from;
	while(cursor<to && !error){
		char* lastcur = cursor;
		rem();
		if(lit(xlb)) skip('[',']');
		else if(decl()) ;
		else if(symName()) {     /* fctn decl */
			union stuff kursor;
			kursor.up = cursor = lname+1;
			newvar('E',2,1,&kursor);
			if( (x=mustFind(cursor, endapp, '[',LBRCERR)) ) {
				cursor=x+1;
				skip('[',']');
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
#endif
}
/*	allocates space for vartab and values, builds vartab
 */
void* link(int from, int to){
	int size;
	void* blob;

	printf("%d %d\n",from,to);
	lngetdata(from,to);
	size = lndata.nvars*sizeof(struct var) + lndata.valsize;
	blob = malloc(size);
	lndolink(from,to,blob,lndata.nvars);
}
/*	tests this mess!
 */
int main(int argc, char **argv) {
	int len;
	len = fileRead("SamplePrograms/first.toc",pr,999);
	link(pr,pr+len);
	return 0;
} 

#if defined(_WIN32)
void printNumber(int nbr)  {
    printf("%d\n", nbr);
}
void myFunction(void (*f)(int))  {
    for(int i = 0; i < 5; i++) 
    {
        (*f)(i);
    }
}
int main(void)  {
    myFunction(printNumber);
    return (0);
}
#endif