#include "toc.h"
#include "string.h"
#include "var.h"
#include "debug.h"
#include "expr.h"
#include "xray.h"

struct funentry fe_last = {0,0,0,0,0};
int watchFunEntry = -1;
int fe_diff(struct funentry *fea, struct funentry *feb){
	return memcmp(fea,feb,sizeof(struct funentry));
}
void fe_cp(struct funentry *to, struct funentry *from){
	memcpy(to,from,sizeof(struct funentry));
}
void xray_stbegin(){
	if(watchFunEntry<0 || verbose_silence)return;
	if(fe_diff(&fe_last,&fun[watchFunEntry])){
		fflush(stdout);
		fprintf(stderr,"\nxray W funentry 2 changing to");
		fe_cp(&fe_last,&fun[watchFunEntry]);
		dumpFunEntry(watchFunEntry);
	}
}
void watch_fe_on(int fe){
	watchFunEntry = fe;
}
void watch_fe_off(){
	watchFunEntry = -1;
}

void m_usage(){
	fprintf(stderr,"   m -- mark, print noticable mark with optional message\n");
	fprintf(stderr,"     _xray \"m\" [\"message\"]\n");
}
DATINT xray_mark(int nargs, DATINT *args){
	char *s = "======================";
	if(nargs>1) s=(char*)args[1];
	fprintf(stderr,"======= %s =======", s);
	return 0;
}

void x_usage(){
	fprintf(stderr,"\n   x -- examine internal data\n");
	fprintf(stderr,"     \"x\",[\"locals[,int]\"|\"globals\"|\"libs\"|\"curobj\"|\"fun\"[,int]]\n");
	fprintf(stderr,"         \"fun\"  prints the table, \"fun\",2  prints 2nd entry\n");
	fprintf(stderr,"         \"locals\"  prints the current frame, \"locals\",4  prints frame 4\n");
}

DATINT xray_examine(int nargs, DATINT *args){
	if(nargs>1){
		if(!strcmp((char*)args[1],"curobj")){
			fprintf(stderr," curobj = %p",curobj);
			return (DATINT)curobj;
		}
		else if(!strcmp((char*)args[1],"fun")){
			if(nargs>2){
				int f = args[2];
				fprintf(stderr,"fun %d",f);
				dumpFunEntry(f);
			}
			else {
				fprintf(stderr,"fun table");
				dumpFunTab();
			}
		}
		else if(!strcmp((char*)args[1],"globals")){
			dumpGlobals();
		}
		else if(!strncmp((char*)args[1],"blob",4)){
			dumpBlobTab();
		}
		else if(!strcmp((char*)args[1],"libs")){
			dumpLibs();
		}
		else if(!strcmp((char*)args[1],"locals")){
			if(nargs>2){
				int f = args[2];
				fprintf(stderr,"var frame %d", f);
				dumpFrame(f);
			}
			else{
				dumpLocals();    // new in var.c
			}
		}
		else if(!strcmp((char*)args[1],"?")){
			x_usage();
		}
		else{
			x_usage();
		}
	}
	else {
		fprintf(stderr,"\nxray code x requires arg");
	}
	return 0;
}

void w_usage(){
	fprintf(stderr,"   w -- watch, report changes to specific data\n");
	fprintf(stderr,"     \"w\", \"frames\"\n");
	fprintf(stderr,"     \"w\" (default) fun[2], // temp for now\n");
/* proposed: _xray "w","[stack|enter]]" 
	enter sets bit VF
*/
}

DATINT xray_watch(int nargs, DATINT *args){
	if(nargs>1){
		if(!strcmp((char*)args[1],"frames")){
			verbose[VFrame]=1-verbose[VFrame];
			return 0;
		}
	}
	watch_fe_on(2);    // temp for now
	return 0;
}

void v_usage(){
	fprintf(stderr,"   v -- verbose, print events [quote the args]\n");
	verbose_usage();   // debug~183
	fprintf(stderr,"     WARNING: it is verbose, use SILENT mode to trim\n");
}

void xray_verbosity(int nargs, DATINT *args){
	if(nargs>1){
		db_verbose((char*)args[1]);	//debug~213
	}
}

void xray_usage(){
	fprintf(stderr," _xray \"m|x|w|v|?\"[,<args>]\n");
	fprintf(stderr," args: (quotes required where shown)\n");
	m_usage();
	x_usage();
	w_usage();
	v_usage();
	fprintf(stderr,"   ?  -- usage, print this usage\n");
}

void xray(int nargs, DATINT *args) {
	if(nargs>0){
		char *a0 = (char*)*args;
		char code = *a0;
		int lineno = countch(apr,appcur,0x0a);
		if(!lineno)lineno = countch(apr,appcur,0x0d);
		fprintf(stderr,"\nxray \"%c\" at %d: ",code,lineno);
		if(nargs>1){
			char *a1 = (char*)*(args+1);
			fprintf(stderr," %s",a1);
		}
		switch(code){
			case 'm':      // mark
				xray_mark(nargs,args);
				break;
			case 'x':    // examine
				xray_examine(nargs,args);
				break;
			case 'v':		// verbosity toggles
				xray_verbosity(nargs,args);
				break;
			case 'w':		// watch
				xray_watch(nargs,args);
				break;
			case '?':
				xray_usage();
				break;
			default:
				fprintf(stderr,"\nUnknown xray code %c\n",code);
				xray_usage();
		}
		fflush(stdout);
	}
	else eset(ARGSERR);
}
