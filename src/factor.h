#ifndef FACTORHDR
#define FACTORHDR
#include "var.h"

void factor();
struct var* obsym(struct varhdr *qvh);
void _pushvar(struct var *v);
char* mustFind( char *from, char *upto, char c, int err );
int _atoi();
Type _konst();

#endif