#ifndef parse_h
#define parse_h

#define xint "int"
#define xchar "char"
#define xcomma ","
#define xnewline "\n"
#define xcmnt "/*"
#define xcmnt2 "//"
#define xstar "*"
#define xsemi ";"

int lit(char *s);
int skip(char l, char r);
int symName();
int symNameIs(char* name);
char* find( char* from, char* upto, char c);
char* mustFind( char *from, char *upto, char c, int err );
char* findEOS( char* x );
void rem();

#endif