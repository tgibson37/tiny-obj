#include "toc.h"

/******* set error unless already set, capture cursor in errat *******/
void eset( int err ){
	if(!error){error=err;errat=cursor;}
}

