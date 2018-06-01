#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>

LOCAL int newlock();

/*------------------------------------------------------------------------
 *  * lcreate  --  create and initialize a lock, returning its id
 *   *------------------------------------------------------------------------
 *    */
int lcreate(void)
{
        STATWORD ps;
        int     lock;
        disable(ps);
        if ((lock=newlock())==SYSERR ) {
                restore(ps);
                return(SYSERR);
        }
        restore(ps);
	kprintf("\nLock is: %d",lock);
        return(lock);
}

/*------------------------------------------------------------------------
 *  * newsem  --  allocate an unused semaphore and return its index
 *   *------------------------------------------------------------------------
 *    */
LOCAL int newlock(){
        int     i;
	//Lock descriptor is index of NLOCKS
        for (i=0 ; i<NLOCKS ; i++) {
                if (locks[i].lstate==LFREE) {
                        locks[i].lstate = LUSED;
			locks[i].lreaders = 0;
                        return(i);
                }
        }
        return(SYSERR);
}
