#ifndef parse_h
#define parse_h

int lit(char *s);
int skip(char l, char r);
int symname();
int symnameis(char* name);
char* find( char* from, char* upto, char c);
char* mustfind( char *from, char *upto, char c, int err );
char* findEOS( char* x );
void rem();

#endif