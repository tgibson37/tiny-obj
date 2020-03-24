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
	struct varhdr *obj;
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
struct od {
  int class; int len; struct var *ocl; void *blob;
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
int nxtvar, vtablen;

/************ functions ************/
int checkBrackets(char *from, char *to);

/*  getters */
int getlen(struct var *v);
struct varhdr* getvarhdr(struct var *v);
//struct varhdr** getobjcell(struct var *v);
char* getvarwhere(struct var *v);
int getvarclass(struct var *v);


void newfun(struct varhdr *vh);
/*	opens a new vartab frame */
void fundone();
/*	pops a vartab frame */
void newvar( int class, Type type, int len, struct var *objclass,
      union stuff *passed, struct varhdr *vh );
/* SITUATION: Declaration is parsed, and its descriptive data known.
 * 	Fill in the var with this data. Allocate value storage unless already
 *	allocated, i.e. pointer to passed data. Copy passed data into allocation.
 *	NOTE: signifantly refactored. */
char* canon_buff(char* first, char* l, char* buff);
/* Canonicalizes the name bracket by f,l inclusive into buff, and returns buff.
	sizeOf buff must be at least VLEN+1. */
void canon(struct var *v);
/* 	fname..lname is full name. Puts canonicalized name into v. If short
 *	enough same as name. If longer first VLEN-1 characters + last character.
 *	The last char has more info than middle chars. */
struct var* addrval_all(char *sym);
/* 	looks up a symbol at one level */
struct var* addrval();
/* 	looks up a symbol pointed to by fname,lname: 
 *	locals, globals, library levels in that order. First hit wins. */
int isfcn(struct var *v);
/*  return true if *v is a function
 */
void dumpVal(Type t, int class, union stuff *val, char lval);
void dumpFunEntry( int e );
void dumpFun();
void dumpVar(struct var *v);
void dumpVarTab();
#endif    /* VARHDR */