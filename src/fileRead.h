#ifndef FILEREADHDR
#define FILEREADHDR

//#include "common.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <libgen.h>
#include "toc.h"

/*************** data ***************/
FILE* fileUnit[MAX_UNIT];
/************* functions ************/

int fileRead(char* name, char* buff, int bufflen);
/*	read the named file into buffer, but not more than bufflen.
	return the number of bytes read, 0 on no such file, 
	-1 on error. */
int fileWrite(char* name, char* buff, int bufflen);
/* write the named file from buffer pr. Return amount written
 *	or -1 on write error. If the named file exists its contents are
 *	truncated to zero before the write. If it doesn't exist it is 
 *	created.
 */
int tcFopen(char* name, char* mode);
/*	open a file returning unit (0..MAX_UNIT), 
 *	else -9, too many, -1 fopen error
 */
int tcFputs(char* str, int unit) ;
/*	put a string from unit returning the length,
 *	else -9 unit not open, -8 bad unit, -2 fputs error. 
 */
int tcFputc(char c, int unit) ;
/*	put a character from unit returning 1 ok,
 *	-9 unit not open, -8 bad unit, -3 fputc error. 
 */
int tcFgets(char* buff, int len, int unit) ;
/*	get a line from unit returning its length including the newline,
 *	else -9 unit not open, -8 bad unit, -4 fgets error
 */
int tcFclose(int unit);
/*	close the file -8 bad unit, -9 already closed, 0 good.
 */
int iProperty(char* file, char* name, int *val, int _default);
/*	set *val to default unless optionally overridden in property file.
 *	Syntax each line: name whiteSpace value newline.
 */
int sProperty(char* file, char* name, char* val, int vlen, char* _default) ;
/*	set *val to default unless optionally overridden in property file.
 *	Syntax each line: name whiteSpace value newline. return -1 for no file,
 *	0 for default being returned, 1 for non-default. 
 */

#endif
