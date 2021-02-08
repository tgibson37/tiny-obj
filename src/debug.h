#ifndef DEBUGHDR
#define DEBUGHDR

#include "common.h"
#include "stack.h"
#include "var.h"
#include "toc.h"

#define BTABSIZE 10
#define BUF_SIZE 80

/************ data ************/
struct brk {
	struct var* var;
	int enabled;
	int hits;
} brktab[BTABSIZE];

/********* tools *******************/
/* look up a symbol without a hit callback */
//struct var* addrval_nohit(char* sym);
/* printf a value ref'd from a struct var, strings truncated at 32 */
//void print_val(struct var *var);
//int num_b(struct brk *b);
/* canonicalize param symbol (f,l) into buf. */
//void canonThis(char* f, char* l, struct var *buf);
/* return brk entry for sym, or false if none */
//struct brk *find_b(char *sym);
/* print description and value of variable v */
//void printVar(struct var *v);

/*	Actions. Some quickies are implemented in db_cmd */
void db_brkset(char *sym);
/* b <sym> */
void db_dump(char* param);
/* d */
void db_info();
/* i */
void db_print(char* param);
/* p <sym>  */
void db_type(char* param);
/* t <sym> */
void db_usage();
/* <default> e.g. ? */
void verbose_clear();
/* v - */
void db_verbose(char* param);
void verbose_usage();
/* v <param> */
char *param();
/*	command parser/dispatcher */
void gdb_b();
/* breakpoint this function to enable using BOTH gdb and tcdb */
int db_cmd();
/* Does one command. Returns the command letter */
void _dbCommands();
/*	Command Loop. Does commands until either x (exit) 
 *	or r,c,n (resume processing) */

/*	BUGOUTs from interpreter. Four standard bugouts. Even the 
 *	1977 8080 code had these. They are an opportunity to take statitics,
 *	debug, or somethink like this debug mode. */
void prbegin();
/*	after link, before interpreter loop */
void prdone();
/* end of whatHappened() */
void tcexit();
/* about to exiting tc */
void appstbegin();
/* beginning of each app statement */
void stbegin();
void xray_stbegin();
/* beginning of each statement. stbegin calls xray_stbegin */
void br_hit(struct var *v);
/* called from symbol lookup if breakpoint set */
void fcn_enter();
/* called from enter when entering functions (calling) */
void fcn_leave();
/* called from enter when leaving functions (returning) */

#endif
