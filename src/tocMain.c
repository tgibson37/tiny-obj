#include "toc.h"
#include "stack.h"
#include "var.h"
#include "fileRead.h"
#include <dlfcn.h>
#include <getopt.h>

#if 0     // needed for dynamic MC load
#include <dlfcn.h>
#endif

/*	main for the toc interpreter. 
 */
int loadMsg=0;
extern char* defaultLibrary;
extern int optind;
extern char* optarg;
//extern char* xendlib;
extern int  (*piMC )(int,int,int*);
int naf(int nargs, int *args);
int dump_mallocs;

char* startSeed="[main();]";
char* ppsPath="./pps";

void tcUsage() {
	printf("Usage: tc [-q] [-d] [-r seed-code] appfile");
	printf("\n-q quiet mode (logo and done msg suppressed)");
	printf("\n-d enables the debugger, which has its own usage print.");
	printf("\n-r run specific seed-code");
	printf("\nThe arg loads the appfile and libraries as follows.");
	printf("\n  If the appfile has NO #include <path to libfile> lines");
	printf("\n  the standard library is also loaded.");
	printf("\n  If the appfile has #include <path to libfile> lines");
	printf("\n  they are loaded. In that case the standard library is not");
	printf("\n  included by default but must be explicitly included in the");
	printf("\n  list if required by the app.");
	printf("\nThe default start of the tinyC app is a main() function,");
	printf("\n  with no arguments. -r changes this default to code, which ");
	printf("\n  can be any function, args permitted, or even a compound");
	printf("\n  statement.");
	printf("\nNo args prints this usage.");
	printf("\n");
}

int loadCode(char* file) {
	int nread = fileRead(file, endapp, EPR-endapp);
	if(nread==0){
		fprintf(stderr,"No such file: %s\n",file);
		exit(1);
	}
	else if(nread<0){
		fprintf(stderr,"Err reading file: %s\n",file);
		exit(2);
	}
	else if(nread>=EPR-endapp ){
		fprintf(stderr,"File to big: %s, ",file);
		fprintf(stderr,"available: %zd bytes\n",EPR-endapp);
		exit(3);
	}
	else if(loadMsg){
		printf("%s loaded\n",file);
	}
	endapp += nread;
	return nread;
}

void markEndlibrary() {
	strcpy(endapp,xendlib);
	endapp+=10;
	apr=endapp;
	if(loadMsg)printf("  endlibrary\n");
}

/*	open the app file, read and process its leading # lines, close 
 *	the file. Return negative on error, else a count of loaded files.
 */
int doIncludes(char* fname) {
	int unit,len,lineno=0,libCount=0;
	char buff[200];
	unit = tcFopen(fname,"r");
	if(unit<0){eset(APPERR);_errToWords();exit(unit);} // lrb
	while(1){
		len = tcFgets(buff,sizeof(buff),unit);
		if(len<0)return len;
		++lineno;
		if(len>0){
			if( buff[len-1]=='\n' || buff[len-1]==0x0d ){
				buff[len-1]=0;
				if(len>1){
					if( buff[len-2]=='\n' || buff[len-2]==0x0d )
						buff[len-2]=0;
				}
			}
		}
		if(!strncmp(buff,"#include ",9)) {
			loadCode(buff+9);  // exit(1) on failure
			++libCount;
		}
		else{
			break;
		}
	}
	tcFclose(unit);
	return libCount;
}

/*	allocate five major areas
 */
int prlen, funlen, vtablen, btablen;
void allocStuff() {
	int err, size;
/*	
 *	program and locals value space
 */
    prlen=PRLEN;
    err = iProperty("pps/tc.prop", "PRLEN", &prlen, PRLEN);
    if(err){
    	fprintf(stderr,"pps/tc.prop err, allocating pr[%d]",PRLEN);
    }
    pr = mymalloc("pr", prlen);
    EPR = pr+prlen;
/*	
 *	function table
 */
    funlen=FUNLEN;
    err = iProperty("pps/tc.prop", "FUNLEN", &funlen, FUNLEN);
    if(err){
    	fprintf(stderr,"pps/tc.prop err, allocating fun[%d]",FUNLEN);
    }
    size = sizeof(struct funentry);
    fun = mymalloc("fun", funlen*size);
    efun=fun+funlen*size;
/*	
 *	expression stack
 */
    stacklen=STACKLEN;
    err = iProperty("pps/tc.prop", "STACKLEN", &stacklen, STACKLEN);
    if(err){
    	fprintf(stderr,"pps/tc.prop err, allocating stack[%d]",STACKLEN);
    }
    stack = mymalloc("stack", stacklen*sizeof(struct stackentry));

/*	
 *	blob table
 */
    btablen=BLOBTABLEN;
    err = iProperty("pps/tc.prop", "BLOBTABLEN", &btablen, BLOBTABLEN);
    if(err){
    	fprintf(stderr,"pps/tc.prop err, allocating blob[%d]",BLOBTABLEN);
    }
    nxtblob = blobtab = mymalloc("blobs", btablen*sizeof(struct blob));
    eblob = blobtab+btablen*sizeof(struct blob);
    curobj = NULL;   // current objects varhdr

/*	
 *	local variable table PLUS space for their values
 */
 	int locnumvars,locdatlen;
    err = iProperty("pps/tc.prop", "LOCNUMVARS", &locnumvars, LOCNUMVARS);
    if(err){
    	fprintf(stderr,"pps/tc.prop err, continuing with local vartab len %d",LOCNUMVARS);
    }
    err = iProperty("pps/tc.prop", "LOCDATLEN", &locdatlen, LOCDATLEN);
    if(err){
    	fprintf(stderr,"pps/tc.prop err, continuing with local locdatlen %d",LOCDATLEN);
    }
    size=sizeof(struct varhdr)+locnumvars*sizeof(struct var)+locdatlen;
    locals = mymalloc("locals", size);
    struct varhdr *vh = (struct varhdr*)locals;
    _newblob("__Locals__",locals);
	memset(vh, 0, size); 
//    vh->vartab = vh->nxtvar = vh->gltab = vh+1;
    vh->gltab =  (struct var*)(vh+1);
    vh->nxtvar = (struct var*)(vh+1);
    vh->vartab = (struct var*)(vh+1);
    vh->datused= (char*)(vh->vartab + LOCNUMVARS);   // also serves as end of vartab
    vh->val =    (char*)(vh->vartab + LOCNUMVARS);   // also serves as end of vartab
    vh->endval = (char*)locals + size;
    vh->sernum = 1;
}

int main(int argc, char *argv[]) {
	int opt,numIncs;
	char *prused;   // end of program text
	int optopt=0;
	char *rArg=NULL;

    while ((opt = getopt(argc, argv, "dlmqvr:")) != -1) {
        switch (opt) {
        case 'l':
        	loadMsg=1;
        	break;
        case 'q':
        	quiet=1;
        	break;
        case 'd': 
        	debug=1; 
        	break;
        case 'm': 
        	dump_mallocs=1; 
        	break;
        case 'v': 
        	verbose[VL]=1; 
        	break;
        case 'r': 
        	rArg=optarg;
        	break;
	    case '?':
	        if (optopt == 'r')
	          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
	        else if (isprint (optopt))
	          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
	        else
	          fprintf (stderr,
	                   "Unknown option character `\\x%x'.\n",
	                   optopt);
	        return 1;
        default:
            fprintf(stderr, "Usage: %s [-d] [-v] [file...]\n", argv[0]);
            exit(1);
        }
    }

	allocStuff();
	if(rArg){
        	*pr = '[';
        	*(pr+1) = 0;
        	strcpy(pr+1,rArg);
        	strcat(pr,"]\n");
	}
	else strcpy(pr,startSeed);
	lpr = endapp = prused = pr+strlen(pr);

	if(loadMsg){
		fprintf(stdout,"Sizes: of pr %d fun %d stack %d var %d\n", 
    			prlen, funlen, stacklen, vtablen);
	}

	cursor = pr;
	curglbl = fun;

/* load app and default or specified libraries, endapp has been set above */
	if(optind >= argc){
		tcUsage();
		exit(1);
	}

	numIncs = doIncludes(argv[argc-1]);
	if(numIncs<0)exit(numIncs);
	if(numIncs==0){
		loadCode(defaultLibrary);
		if(loadMsg)printf("  (default)\n");
	}
	markEndlibrary();
	/* load the app */
	loadCode(argv[argc-1]);
// initialize: locals use legacy vartab, toclink() will do globals in a blob
	error=0;
	curobj=canobj=NULL;
	curfun = fun-1;
	logo(); 
	toclink();
	cursor=pr;
	prbegin();
	st();   /* <<<== executes statement above, line 15 */
	prdone();
	whatHappened();
    return 0;
}
void* mymalloc(char *name, int size){
	void *m;
	m=malloc(size);
#if 0          // undocumented tc-dbg -m option
	if(dump_mallocs)fprintf(stderr,
			"\nMALLOC %s size %d at %td..%td (%p..%p)"
			,name,size, (ptrdiff_t)m,(ptrdiff_t)m+size, m,m+size);
#else
	if(dump_mallocs)fprintf(stderr,
 			"\nMALLOC %s size %d at %p-%p",name,size,m,m+size);
#endif
	return m;
}
