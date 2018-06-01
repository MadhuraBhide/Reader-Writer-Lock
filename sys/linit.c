#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>

struct lentry locks[NLOCKS];

void linit(){
	int i,j;
	struct lentry *lptr;
	for(i=0 ; i<NLOCKS ; i++) {      /* initialize locks */
		(lptr = &locks[i])->lstate = LFREE;
    		lptr->lqtail = 1 + (lptr->lqhead = newqueue());
		lptr->lprio = -1;
		lptr->lreaders = 0;
		lptr->ltype = NONE;
		for(j=0;j<NPROC;j++){
			lptr->pidheld[j] = 0;
		}
	}	
}
