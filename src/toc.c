#include "toc.h"

/******* set error unless already set, capture cursor in errat *******/
void eset( int err ){
	if(!error){error=err;errat=cursor;}
}
/* print from..to inclusive
 */
void pft(char *from, char *to ) {
        while(from <= to) printf("%c",*(from++));
}
void pftl(char *label, char* from, char* to){
	printf("%s",label);
	pft(from,to);
}
int countch(char *from, char *to, char ch) {
//ps("ch =");pn(ch);
    int i;
    char *c;
    for (i=0,c = from; c <= to; c++) {
        i += (*c == ch);
    }
    return i;
}
void pn(int n){ printf(" %d",n);}
void ps(char* s){ printf("%s",s);}
void pl(char* s){ printf("\n%s",s);}
void prlineno() { 
	pn( 1+countch(pr,cursor,'\n') ); 
}
void dumpcurl(char *l){
	pl(l); ps("line");
	prlineno();
	pftl("-->",cursor,cursor+9);
	ps("<--");
}
