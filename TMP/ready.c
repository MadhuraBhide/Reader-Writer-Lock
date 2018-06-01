/* ready.c - ready */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
/*------------------------------------------------------------------------
 * ready  --  make a process eligible for CPU service
 *------------------------------------------------------------------------
 */
int ready(int pid, int resch)
{
	register struct	pentry	*pptr;

	if (isbadpid(pid))
		return(SYSERR);
	pptr = &proctab[pid];
	pptr->pstate = PRREADY;
	int my_prio = 0;
	if(pptr->pinh!=0) my_prio = pptr->pinh;
	else my_prio = pptr->pprio;
	//Insert with key as pinh in ready queue if pinh is non-zero, else pprio
	insert(pid,rdyhead,my_prio);
//	insert(pid,rdyhead,pptr->pprio);
	if (resch)
		resched();
	return(OK);
}
