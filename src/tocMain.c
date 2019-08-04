#include <dlfcn.h>
#include "toc.h"

/*	main for the tc interpreter. Their are two mains. The other, test.c
 * 	loads a set regression tests which can be run individually or collectively.
 */
int loadMsg=0;
extern char* defaultLibrary;
extern int optind;
extern char* optarg;
//extern char* xendlib;
extern int  (*piMC )(int,int,int*);
int naf(int nargs, int *args);

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

#if 0
// Get the function pointer to the function
void* getFcnPtr(void* lib, const char* fcnName) {
    void* fptr = dlsym(lib, fcnName);
    if (!fptr) {
        fprintf(stderr, "Could not get function pointer for %s\n  error is: %s\n\n", fcnName, dlerror());
        return NULL;
    }
    return fptr;
}

void loadMC(char* libName) {
	char fileName[1000];

// Open the dynamic library
	strcpy(fileName,"./lib");
	strcat(fileName,libName);
	strcat(fileName,".so");
    void* libMC = dlopen(fileName,  RTLD_LAZY | RTLD_GLOBAL);
    if (!libMC) {
        fprintf(stderr, "Could not open %s\n",fileName);
        fprintf(stderr, "dlerror: %s\n", dlerror());
        exit(1);
    }
// Set the plugInMC fcn pointer
    piMC = getFcnPtr(libMC, "plugInMC");
	if(!piMC) {
        fprintf(stderr, 
        	"Protocol: plug in MC %s needs plugInMC function\n"
        	,libName);
        exit(1);
    }
    if(loadMsg)printf("MC: %s loaded\n",fileName);
// set callback to eset
	void (*register_eset)(void(*)());   // Prototype of fcn in .so
	register_eset = dlsym(libMC, "register_eset");
	if(register_eset == NULL){
		fprintf(stderr,
			"Error: %s needs register_eset function\n"
			,fileName);
		exit(1);
	}
	(*register_eset)(&eset);   //calls to .so's reg_fcn
    if(loadMsg)printf("callback function eset registered");
}
#endif

int loadCode(char* file) {
	int nread = fileRead(file, endapp, EPR-endapp);
	if(nread==0){
		fprintf(stderr,"No such file: %s\n",file);
		exit(1);
	}
	else if(nread<0){
		fprintf(stderr,"Err reading file: %s\n",file);
		exit(1);
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
	int unit,len,lineno=0,libCount=0,loadCount=0;
	char buff[200];
	unit = tcFopen(fname,"r");
//	if(unit<0)return unit;
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
#if 0
		else if(!strncmp(buff,"#loadMC ",8)) {
			loadMC(buff+8);  // exit(1) on failure
			++loadCount;
		}
#endif
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
    pr = malloc(prlen);
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
    fun = malloc(funlen*size);
    efun=fun+funlen*size;
/*	
 *	expression stack
 */
    stacklen=STACKLEN;
    err = iProperty("pps/tc.prop", "STACKLEN", &stacklen, STACKLEN);
    if(err){
    	fprintf(stderr,"pps/tc.prop err, allocating stack[%d]",STACKLEN);
    }
    stack = malloc(stacklen*sizeof(struct stackentry));

/*	
 *	blob table
 */
    btablen=BLOBTABLEN;
    err = iProperty("pps/tc.prop", "BLOBTABLEN", &btablen, BLOBTABLEN);
    if(err){
    	fprintf(stderr,"pps/tc.prop err, allocating blob[%d]",BLOBTABLEN);
    }
    nxtblob = blobtab = malloc(btablen*sizeof(struct blob));
    eblob = blobtab+btablen*sizeof(struct blob);

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
    locals = malloc(size);
    struct varhdr *vh = (struct varhdr*)locals;
    _newblob("_Locals",locals);
	memset(vh, 0, size); 
    vh->vartab = vh->nxtvar = vh->gltab = vh+1;
    vh->val = vh->datused = vh->vartab + LOCNUMVARS;   // also serves as end of vartab
    vh->endval = (char*)locals + size;
fprintf(stderr,"\n--- %s %d ---\n",__FILE__,__LINE__);
dumpVarTab(locals);
}

int main(int argc, char *argv[]) {
	int opt,optopt,numIncs;
	char *prused;   // end of seed

	allocStuff();
	strcpy(pr,startSeed);
	lpr = endapp = prused = pr+strlen(startSeed);

    while ((opt = getopt(argc, argv, "lqdvr:")) != -1) {
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
        case 'v': 
        	verbose[VL]=1; 
        	break;
        case 'r': 
        	*pr = '[';
        	strcpy(pr+1,optarg);
        	lpr = endapp = prused = pr+strlen(optarg)+3;
        	*(endapp-2) = ']';
        	*(endapp-1)='\n';
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
//	prused = endapp+10;  /* a little slack */
//	nxtvar = vartab;
//	nxtstack = 0;
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
