#include <kernel.h>
#include <stdio.h>
#include <q.h>
#include <proc.h>
#include <lock.h>

void Priority_inherit(int lock);
void Max_lprio(int lock);
int lock(int ldes1, int type, int priority) {
	STATWORD ps;
        struct  lentry  *lptr;
        struct  pentry  *pptr;
	struct pentry *temp_ptr;

	//Acquire a lock atomically
        disable(ps); 
	lptr = &locks[ldes1];
	pptr = &proctab[currpid];
	
	//Similar to Xinu semaphore implementation, return error if lock is in a bad state
        if (isbadlock(ldes1) || pptr->lockheld[ldes1] != NONE) {
                restore(ps);
                return(SYSERR);
        }
	//If some other process deletes a lock while other processes are waiting, return error to such processes
	if(pptr->lockheld[ldes1] == DELETED){
		restore(ps);
		return(SYSERR);
	}
	int ptr,flag = 0;
	//Flag is used to indicate if process should wait or acquire a lock straightaway
	//If Lock Type is NONE, which means no process has acquired a lock, hence a process can acquire it.
        if (lptr->ltype == NONE) {
		flag = 0;
	}
	//Lock Type WRITE indicates writer has acquired a lock, in which case, all readers and writers must wait.
	else if(lptr->ltype == WRITE){
		flag = 1;
	}
	//If Reader has already acquired a lock, admit other reader if there is no other higher priority writer waiting in the queue.
	else if(lptr->ltype==READ && type == READ){
		ptr = q[lptr->lqtail].qprev;
		temp_ptr = &proctab[ptr];
		while(priority < q[ptr].qkey){
			if(temp_ptr->plocktype == WRITE){
				flag = 1;
			}	
			ptr = q[ptr].qprev;
			temp_ptr = &proctab[ptr];
		}
	}
	//If reader has acquired a lock, and writer is trying to acquire it, it must wait
	else if(lptr->ltype==READ && type == WRITE){
		flag = 1;
	}
	else if(lptr->ltype==DELETED){
		restore(ps);
		return(SYSERR);
	}
	//If a process has to wait, put it in PRWAIT state, insert it in lock queue and update variables.
	if(flag){
                pptr->pstate = PRWAIT;
		pptr->lockid = ldes1;
                insert(currpid,lptr->lqhead,priority);
		pptr->plocktype = type;
		//if(type == READ) lptr->lreaders = lptr->lreaders + 1;
		pptr->plockwait = ctr1000;
                pptr->pwaitret = OK;
		lptr->pidheld[currpid] = type;
        	pptr->lockheld[ldes1] = type;
		Priority_inherit(ldes1);
                Max_lprio(ldes1);
		//kprintf("Before resched");
                resched();
                restore(ps);
                return(pptr->pwaitret);
	}
	//Else acquire a lock
	else{
		if(type == READ){
	   		lptr->ltype = READ;
	   		lptr->lreaders = lptr->lreaders + 1;
	}
		else{
	   		lptr->ltype = WRITE;
		}	
		
		lptr->pidheld[currpid] = type;
        	pptr->lockheld[ldes1] = type;
        	restore(ps);
        	return(OK);	
	}
}
//Function for a process holding a lock to inherit priority of waiting processes
void Priority_inherit(int lock){
	struct  lentry  *lptr = &locks[lock];
        struct  pentry  *pptr1;
	struct  pentry  *pptr2;
	int i,prio1,prio2;
	pptr1 = &proctab[currpid];
	//Check priorities of processes waiting for lock and inherit priority
	for(i=0;i<NPROC;i++){
		if((lptr->pidheld[i] == READ || lptr->pidheld[i] == WRITE) && proctab[i].lockid != lock){
				break;
		}
	}
	pptr2 = &proctab[i];
	//Consider pinh of waiting processes
	if(pptr1->pinh != 0) prio1=pptr1->pinh;
	else prio1=pptr1->pprio;
	if (pptr2->pinh != 0) prio2=pptr2->pinh;
	else prio2=pptr2->pprio;
	//Inherit priority
	if(prio2 < prio1){
		pptr2->pinh = prio1;	
	}
	pptr1 = pptr2;
	lock = pptr2->lockid;
	lptr = &locks[lock];
	//For transitivity, check inherited priorities of other locks too
	while(pptr2->lockid != -1){
		for(i=0;i<NPROC;i++){
                	if((lptr->pidheld[i] == READ || lptr->pidheld[i] == WRITE) && proctab[i].lockid != lock){
                                break;
                	}
       		}
        	pptr2 = &proctab[i];
        	if(pptr1->pinh != 0) prio1=pptr1->pinh;
        	else prio1=pptr1->pprio;
        	if (pptr2->pinh != 0) prio2=pptr2->pinh;
        	else prio2=pptr2->pprio;

        	if(prio2 < prio1){
                	pptr2->pinh = prio1;
        	}	
        	pptr1 = pptr2;
        	lock = pptr2->lockid;
        	lptr = &locks[lock];
	}
}
//Function to calculate lprio
void Max_lprio(int lock){
	struct  lentry  *lptr = &locks[lock];
	int prio = -1,temp_prio;
	int ptr = q[lptr->lqtail].qprev;
	while(ptr!=lptr->lqhead){
		if(proctab[ptr].pinh != 0) temp_prio=proctab[ptr].pinh;
		else temp_prio = proctab[ptr].pprio;
		if(temp_prio > prio){
			prio = temp_prio;
		}
		ptr = q[ptr].qprev;
	}	
	lptr->lprio = prio;
}	
