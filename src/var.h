#ifndef VARHDR
#define VARHDR
#include "debug.h"

/************ data ************/
/*	Length of a canon'd variable */
//#define VLEN 8, moved to common.h to avoid circular def'n 

/* a fun entry */
struct funentry {
	struct var *fvar, *evar;
	char *datused;
	struct varhdr *uobj;
  struct varhdr *aobj;
};

/*	fun table, malloc'd in tcMain. This table devides the vartab
	 into frames for libs, globals, function locals */
struct funentry *fun;
struct funentry *curglbl, *curfun, *efun;

/*	var is an entry in the vartab. The extra byte in name is for a NULL,
 *	so that var.name is a string. There are three types of vars: vd, cd,
 * 	and od, describing a variable, class, and object respectively.
 *	Union vdcd can be any of the three.
 */
struct cd {
  char parent[VLEN+1]; int abst; char* where; struct varhdr *blob;
};
struct vd {
  int class; int len; int brkpt; union stuff value;
};
/*  **blob because it must support an array of *blob */
struct od {
  int class; int len; struct var *ocl; struct varhdr **blob;
};
union vdcd {
  struct vd vd; struct cd cd; struct od od;
};
struct var{
  char name[VLEN+1]; Type type; union vdcd vdcd; 
};
/* vartab is the variable table, malloc'd in tcMain. It points to
the first entry of the variable table. */
struct var *vartab;
//int nxtvar;
int vtablen;

/************ functions ************/
int checkBrackets(char *from, char *to);

/*  getters */
int getlen(struct var *v);
//struct varhdr* getvarhdr(struct var *v);
//struct varhdr** getobjcell(struct var *v);
char* getvarwhere(struct var *v);
int getvarclass(struct var *v);
void openVarFrame(struct varhdr *vh);
void fundone();
void newvar( int class, Type type, int len, struct var *objclass,
      union stuff *passed, struct varhdr *vh );
char* canon_buff(char* first, char* l, char* buff);
void canon(struct var *v);
struct var* addrval_all(char *sym);
struct var* addrval();
int isfcn(struct var *v);
int inBlob(void* p);
void dumpBlobTab();
void allocSpace(struct var *v, int amount, struct varhdr *vh);
void dumpVal(Type t, int class, union stuff *val, char lval);
void dumpFunEntry( int e );
void dumpFunTab();
void dumpVar(struct var *v);
void dumpVarTab(struct varhdr *vh);
void dumpGlobals();
void dumpLocals();
void dumpLibs();
void dumpVft(struct var *from,struct var *to);
#endif    /* VARHDR */
