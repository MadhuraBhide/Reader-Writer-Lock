/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>
/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */

extern void Priority_inherit(int lock);
extern void Max_lprio(int lock);

SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	pptr->pprio = newprio;
	int i;
	/* Update pinh if priority of a process holding or waiting on a lock is being changed */
	for(i=0;i<NLOCKS;i++){
		if(pptr->lockheld[i] == READ || pptr->lockheld[i] == WRITE){
			if(pptr->pstate == PRWAIT){
				//If waiting, update lprio,pinh and recalcute inherited priority of a process holding a lock 
				Max_lprio(i);
                        	int k;
                        	for(k=0;k<NPROC;k++){
                                	if((locks[i].pidheld[k] == READ || locks[i].pidheld[k] == WRITE) && proctab[k].lockid != i){
                                        break;
                               		}
                        	}
                        	update_pinh(k);
                        	Priority_inherit(i);
			}
			else{
				//If the process whose priority is beig changed is itself holding a lock, update inherited priority
				Priority_inherit(i);
			}
		}
	}
	restore(ps);
	return(newprio);
}
