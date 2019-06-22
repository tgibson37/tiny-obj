#ifndef link_h
#define link_h

#define xcomma ","
#define xsemi ";"
#define xlb "["
#define xrb "]"

void* link(int from, int to);

union stuff { char uc; int ui; void* up; };
#define VLEN 16
typedef enum type {Err,Char,Int,CharStar} Type;
struct var { 
	char name[VLEN+1]; int klass; Type type; int len; int brkpt;
	union stuff value; 
};

/* variable table */
struct var *vartab;
int nxtvar, vtablen;

#endif  /* link_h */