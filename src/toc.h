#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

/* Modified tc.h
 */

#define xif "if"
#define xelse "else"
#define xint "int"
#define xchar "char"
#define xwhile "while"
#define xreturn "return"
#define xbreak "break"
#define xendlib "endlibrary"
#define xclass "class"
#define xabstract "abstract"
#define xextends "extends"
#define xnew "new"
#define xdelete "delete"
#define xr "r"
#define xg "g"
#define xlb "["
#define xrb "]"
#define xlpar "("
#define xrpar ")"
#define xcomma ","
#define newline "\n"
#define xcmnt "/*"
#define xcmnt2 "//"
#define xstar "*"
#define xsemi ";"
#define xpcnt "%"
#define xslash "/"
#define xplus "+"
#define xminus "-"
#define xlt "<"
#define xgt ">"
#define xnoteq "!="
#define xeqeq "=="
#define xeq "="
#define xge ">="
#define xle "<="
#define xnl "\n"
#define xvarargs "..."



#define ECHO 1
#define ESCAPE 27 // lrb
#define CTRLC 3 // lrb
#define UPARROW -65
#define DOWNARROW -66
#define RIGHTARROW -67
#define LEFTARROW -68

#define VLEN 15
#define BLOBTABLEN 100
#define PRLEN 30000
#define STACKLEN 100
#define FUNLEN 100
#define MAX_UNIT 10
#define LOCNUMVARS 100
#define LOCDATLEN 1000

/* debug and verbosity tags */
#define VE 0
#define VL 1
#define VS 2
#define VP 3
#define VV 4
char verbose[8];
int debug;
int quiet;

/* error tags */
#define STATERR      1
#define CURSERR      2
#define SYMERR       3
#define RPARERR      5
#define RANGERR      6
#define CLASERR      7
#define TYPEERR      8
#define SYNXERR      9
#define LVALERR      14
#define POPERR       15
#define PUSHERR      16
#define TMFUERR      17
#define TMVRERR      18
#define TMVLERR      19
#define LINKERR      20
#define ARGSERR      21
#define LBRCERR      22
#define RBRCERR      23
#define MCERR        24
#define SYMERRA      26
#define EQERR		 27
#define PTRERR		 28
#define APPERR	     29 // lrb
#define DIVERR		 30
#define TMBLOBERR    31
#define CLASSERR	 32
#define WHERERR      33
#define FREEZERR     96
#define LIMITERR     97
#define EXIT         98
#define KILL         99

/*	All tc data are ints or chars. Pointers are an int index into pr. 
 *	stuff is the typless holder of any value.
 */
union stuff { char uc; ptrdiff_t ui; void* up; };

typedef enum type {Err,Char,Int,CharStar} Type;

/* a stack entry */
struct stackentry { 
	int class; int lvalue; Type type; 
	union stuff value; 
};
/* the stack */
struct stackentry *stack;
int nxtstack, stacklen;

/* a fun entry */
struct funentry {
	struct var *fvar, *evar;
	char *datused;
};
/* fun table */
struct funentry *fun;
struct funentry *curglbl, *curfun, *efun;

//struct var { 
//	char name[VLEN+1]; int class; Type type; int len; int brkpt;
//	union stuff value; 
//};
//  MUST add after -> or ). before value/class/etc    vdcd.vd.
//  but NOT before name or type.

struct cd {
  char parent[VLEN+1]; char child[VLEN+1]; int abst; char* where;
};
struct vd {
  int class; int len; int brkpt; union stuff value;
};
struct od {
  struct var *cls; struct varhdr *blob;
};
union vdcd {
  struct vd vd; struct cd cd; struct od od;
};
struct var{
  char name[VLEN+1]; Type type; union vdcd vdcd; 
};
// struct var *vt = vh->vartab;			// vh == blob
// vt->var.vdcd.od.blob  				// <<== typical

/* blob header */
struct varhdr {
	struct var *vartab; struct var *gltab; struct var *nxtvar; 
	char *val; char *endval; char *datused; 
};
/*	locals, all of them, 
 *	candidate for current object, current object
 */
struct varhdr *locals, *canobj, *curobj;
/* sizes needed for each vartab, values area */
struct lndata { int nvars; int valsize; } lndata;

/* blob table, f,t scope the text for enter to set curclass */
struct blob {
	char name[VLEN+1]; char *f; char *t; struct varhdr *varhdr;
};
struct blob *blobtab, *nxtblob, *eblob;
void newblob(char *blob);

/* most recent function entered */
char fcnName[VLEN+1];
void saveName();

/* program space */
char *pr;
char *lpr, *apr, *endapp, *EPR;

/* EPR is end of program SPACE. 
 *	pr starts with startSeed, then libs, then app
 *	lpr is start of libraries
 *	apr is start of application program
 *	endapp is end of ALL program text
 *	EPR is pointer just beyond last byte of pr array 
 */

/************ Globals **************/
int error, leave, brake;
char* fname;
char* lname;
char* cursor;
char* stcurs;
char* errat;
int obsize, vclass, alen;
int traceMode;
extern char* ppsPath;

FILE* fileUnit[MAX_UNIT];
int tcFopen(char* name, char* mode);
int tcFputs(char* str, int unit);
int tcFputc(char c, int unit);
int tcFgets(char* buff, int len, int unit);
int tcFclose();
void stbegin();
void fcn_enter();
void fcn_leave();
char escKey();
int iProperty(char* file, char* name, int *val, int _default);
int sProperty(char* file, char* name, char* val, int vlen, char* _default);
int typeToSize( int class, Type type );
int topdiff();
ptrdiff_t toptoi();
struct stackentry* popst();
void pushst( int class, int lvalue, Type type, union stuff *value );
void pushk(ptrdiff_t datum);
void pushone();
void pushzero();
void pushPtr(ptrdiff_t datum);
void eset( int err );
void ps(char* s);
void pl(char* s);
int  pn(int n);
void pc(char c);
char* find( char *from, char *upto, char c);
void newfun(struct varhdr *vh);
void fundone();
void newvar(int class, Type type, int len, union stuff *passed, struct varhdr *vh );
struct var* addrval();
void canon(struct var *v);
int quit();
void st();
void machinecall();
char* typeToWord(Type t);
void dumpVal(Type t, int class, union stuff *val, char lval);
void dumpFun();
void dumpLine();
void dumpStuff(int v, int class, Type type);
void dumpStackEntry(int e);
void dumpStack();
void dumpPopTop();
void dumpTop();
void dumpVar(struct var *v);
void dumpVarTab(struct varhdr *vh);
void dumpHex( void* where, int len );
void dumpState();
void dumpName();
void dumpft(char*,char*);
void put_int(char *where, ptrdiff_t datum);
ptrdiff_t  get_int(char *where);
void put_char(char *where, char datum);
char get_char(char *where);
int fileRead(char*filename, char* buffer, int bufflen);
int fileWrite(char*filename, char* buffer, int bufflen);
void readTheFiles(int argc, char *argv[], int optind);
int countch(char*,char*,char);
char* fchar(char*);
char* lchar(char*);
struct var* addrval_all(char*);
struct var* _addrval(char *sym, struct var *first, struct var *last);
struct var* addr_obj(struct varhdr *vh);
void showLine(char*);
char getch_(int echo);
int kbhit(void);
void logo();
void tclink();
void prbegin();
void prdone();
void whatHappened();
void tcUsage();
void br_hit(struct var *v);
void pft(char *from, char *to );
  /* All these do is prevent warnings from compiler */
ptrdiff_t Mchrdy();
ptrdiff_t Mgch(int,ptrdiff_t*);
int _asgn();
int _reln();
int _expr();
int _term();
void _factor();
char* _canon(char* first, char* l, char* buff);
void _errToWords();
void allocStuff();
void _rem();
int _decl(struct varhdr *vh);
int _lit(char*);
void varalloc(Type type, union stuff *vpassed, struct varhdr *vh);
int symName();
int charIn(char c, char *choices );
void pFmt(char *fmt, ptrdiff_t *args);
int _skip(char l, char r);
struct varhdr* lnlink(char *from, char *to, char *blobName);
void* mymalloc(char *name, int size);
int dump_mallocs;
void stuffCopy( union stuff *to, union stuff *from );
struct var* _isClassName();
void newref(struct var *cls, struct varhdr *vh);
char* _mustFind( char *from, char *upto, char c, int err );
void toclink();
void _newblob(char* name, void* blob);

