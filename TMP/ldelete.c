#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>
/*------------------------------------------------------------------------
 *  * sdelete  --  delete a semaphore by releasing its table entry
 *   *------------------------------------------------------------------------
 *    */
int ldelete(int lock)
{
        STATWORD ps;
        int     pid;
        struct  lentry  *lptr;

        disable(ps);
        if (isbadlock(lock) || locks[lock].ltype==NONE || proctab[currpid].lockheld[lock] == DELETED) {
                restore(ps);
                return(SYSERR);
        }
        lptr = &locks[lock];
        if (nonempty(lptr->lqhead)) {
                while( (pid=getfirst(lptr->lqhead)) != EMPTY)
             	 {
                    proctab[pid].lockheld[lock] = DELETED;
		    proctab[pid].pwaitret = DELETED;
		    dequeue(pid);
                    ready(pid,RESCHNO);
                  }
        }
	int i;
	for(i=0;i<NLOCKS;i++){
		if (locks[lock].pidheld[i] == READ || locks[lock].pidheld[i] == WRITE) {
			locks[lock].pidheld[i] = NONE;
		}
	}
	lptr->lstate = LFREE;
        lptr->ltype = NONE;
        lptr->lreaders = 0;
        restore(ps);
        return(OK);
}
