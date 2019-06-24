#ifndef toc_h
#define toc_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* parse tokens */
#define xabstract "abstract"
#define xchar "char"
#define xclass "class"
#define xcmnt "/*"
#define xcmnt2 "//"
#define xcomma ","
#define xextends "extends"
#define xint "int"
#define xlb "["
#define xnewline "\n"
#define xrb "]"
#define xsemi ";"
#define xstar "*"

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
#define EXIT         98
#define KILL         99

int error;
char* errat;
char *cursor;
char *endapp;
char *fname;
char *lname;
//char *;

char pr[9999];

void eset( int err );
void pft(char *from, char *to );
void pftl(char *label, char* from, char* to);
int countch(char *from, char *t, char ch);
void pn(int n);
void ps(char* s);
void pl(char* s);
void prlineno();


#endif
