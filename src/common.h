#ifndef COMMONHDR
#define COMMONHDR
#include <stddef.h>

/* These must come first to avoid circular def'n among headers */

#define VLEN 15
/*	Length of a canon'd symbol. varEntry adds one more byte to 
 *	make it a string. Multiple of 8 minus 1 recommended.*/

#if 0           // choose one
#define DATINT int
#define FMTINT "%d"
#endif

#if 0           // choose one
#define DATINT long
#define FMTINT "%ld"
#endif

#if 1           // choose one
#define DATINT ptrdiff_t
#define FMTINT "%td"
#endif
/*	Choose one and only one of above for tc integer precision.
 *	Some choices will generate warnings. ptrdiff_t is
 *	recommended if it is supported by your platform. 
 */

union stuff { char uc; DATINT ui; void* up; };
/*	All tc data are ints or chars. Pointers are an int index into pr. 
 *	stuff is the typeless holder of any value.
 */
typedef enum type {Err,Char,Int,CharStar} Type;
/*	The type of a constant and stackEntry. Err shouldn't happen. */

#endif