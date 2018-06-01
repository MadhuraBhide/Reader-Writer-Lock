/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*----------------------------------------------------------------------n
i kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */

extern void Priority_inherit(int lock);
extern void Max_lprio(int lock);
extern void update_pinh(int pid);
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev,i,my_lock;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	/* Update inherited priority, pinh and lprio if a process in a wait queue of lock is being killed */
			semaph[pptr->psem].semcnt++;
			dequeue(pid);
			my_lock = proctab[pid].lockid;
			/* Reset lock variables */
			proctab[pid].lockid = -1;
			proctab[pid].lockheld[my_lock] = NONE;
			locks[my_lock].pidheld[pid] = NONE;
			proctab[pid].pinh = 0;
			Max_lprio(my_lock);
			int i;
			for(i=0;i<NPROC;i++){
                		if((locks[my_lock].pidheld[i] == READ || locks[my_lock].pidheld[i] == WRITE) && proctab[i].lockid != lock){
                                	break;
               			}
        		}
        		update_pinh(i);
			Priority_inherit(my_lock);
		
	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
