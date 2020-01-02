#ifndef LINKHDR
#define LINKHDR

#include "var.h"

struct lndata { int nvars; int valsize; } lndata;

/* links the loaded program. eset(LINKERR) on failure.
 */
void toclink();

/*	links a class given its *var. Returns pointer to the malloc'd
 *	blob which contains the varhdr. The blob has space for the var
 *	table and class and object data.
 */
struct varhdr* classlink(struct var *isclvar);

#endif
