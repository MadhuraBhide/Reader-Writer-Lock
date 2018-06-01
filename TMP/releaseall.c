#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>
#include <sleep.h>

extern void Priority_inherit(int lock);
extern void Max_lprio(int lock);

extern unsigned long ctr1000;
LOCAL void make_ready(int lock,int pid,int type);
LOCAL void admit_other_readers(int lock);
void update_pinh(int pid);

int releaseall (int numlocks, long ldes1,...){
	int i,val;
	long a;
	STATWORD ps;
	disable(ps);
	//Retrive lock descriptors passed to a function from stack, first argument is ldes1, followed by 'numlocks' arguments
	for(i=0;i<numlocks;i++){
		a = (long)*(&ldes1+ i);
		//Call release function for each lock descriptor 
		val = release(a);
		if (val == SYSERR){
			return(SYSERR);
		}
	}
	resched();
	restore(ps);
	return(OK);
}
int release(int lock)
{
	register struct pentry *proc_ptr;
        register struct lentry *lptr;
	int ptr, proc_sched,temp_ptr;
	int type[NLOCKS],pid[NLOCKS];
	int wait_time[NLOCKS];
	//Similar to Xinu semaphore
        if (isbadlock(lock) || (lptr= &locks[lock])->ltype==NONE && proctab[currpid].lockheld[lock]==DELETED) {
                return(SYSERR);
        }
	//Update variables
	lptr->pidheld[currpid] = NONE;
        proctab[currpid].lockheld[lock] = NONE;
	update_pinh(currpid);
	if(lptr->ltype == READ && lptr->lreaders > 1){
                lptr->lreaders = lptr->lreaders - 1;
                return(OK);
        }

	//If Writer or last reader is releasing a lock, check lock priorities to select next process to which lock should be given
        if (lptr->ltype == WRITE || lptr->lreaders == 1){	
		//If no process is waiting in lock queue, reset variables and return
			if((lptr->lqtail = 1 + lptr->lqhead) && isempty(lptr->lqhead)){
        		lptr->lstate = LFREE;
        		lptr->ltype = NONE;
        		lptr->lreaders = 0;
			return(OK);
		}
		if(lptr->lreaders == 1) lptr->lreaders = 0;
		//Else select highest priority(lock priority) process from waiting queue
		ptr = q[lptr->lqtail].qprev;
		proc_ptr = &proctab[ptr];
		int get_prio = q[ptr].qkey;
		temp_ptr = ptr;
		ptr = q[ptr].qprev;
		//If no tie between highest priorities, give lock to highest priority process
		if(q[ptr].qkey<get_prio){
			if(proc_ptr->plocktype == READ){
				//If a lock is being given to a reader, admit other readers too
				//Policy for giving lock to other readers is defined in function admit_other_readers
				admit_other_readers(lock);
			}
			else{
				//If a lock is being given to a writer, put writer process in ready queue and update variables
				make_ready(lock,temp_ptr,WRITE);}
		}
		//In case of tie, check waiting times.
		else{
			int k = 0;
			pid[k] = q[lptr->lqtail].qprev;
			wait_time[k] = ctr1000 - proc_ptr->plockwait;
			type[k] = proc_ptr->plocktype;
			proc_ptr = &proctab[ptr];
			//Update local variable arrays 
                	while(q[ptr].qkey==get_prio && (ptr!=lptr->lqhead)){
				k++;
				pid[k] = ptr;
				wait_time[k] = ctr1000 - proc_ptr->plockwait;
	                        type[k] = proc_ptr->plocktype;
                        	ptr = q[ptr].qprev;
				proc_ptr = &proctab[ptr];
               		
			}
			int j,index1=-1,index2=-1,highest_reader = 0,highest_writer=0;
			for(j=0;j<=k;j++){
				if(type[j] == READ){
					//Select highest priority reader
					if(wait_time[j] > highest_reader){
						highest_reader = wait_time[j];
						index1 = j;
					}
				}
				else if(type[j] == WRITE){
					///Select highest priority writer
                                        if(wait_time[j] > highest_writer){
                                                highest_writer = wait_time[j];
                                                index2 = j;
                                        }
                                }
			}
			//If no such reader exists, select highest priority writer
			if(index1 == -1){
				proc_sched = pid[index2];
				make_ready(lock,proc_sched,WRITE);
			}
			//If no such writer exists, select highest priority reader
			else if(index2 == -1){
				proc_sched = pid[index1];
				admit_other_readers(lock);
				make_ready(lock,proc_sched,READ);
			}
			//In case of tie, consider waiting times
			else{
				if((ctr1000 - wait_time[index1]) > (ctr1000 - wait_time[index2])){
					proc_sched = pid[index1];
                                        admit_other_readers(lock);
				}
				else{
					if((ctr1000 - wait_time[index2]) - (ctr1000 - wait_time[index1]) <= 500){
					proc_sched = pid[index1];
					admit_other_readers(lock);
					}
					else{
					proc_sched = pid[index2];
					make_ready(lock,proc_sched,WRITE);
					}
				}	
			}
		}
	}
	else{
		return(SYSERR);
	}
	return(OK);
}
//Function to select other readers by comparing their lock priority with priority of waiting writer when a reader is selected 
LOCAL void admit_other_readers(int lock){
	int highest_prio,temp_ptr;
	struct lentry *lptr = &locks[lock];
	int my_ptr = q[lptr->lqtail].qprev;
	struct pentry *ptr = &proctab[my_ptr];
	float writer_wait = 0;
	while(ptr->plocktype != WRITE && (my_ptr != lptr->lqhead)){
		 my_ptr = q[my_ptr].qprev;
		 ptr = &proctab[my_ptr];
	}
	if(ptr->plocktype == WRITE){
		writer_wait = ctr1000 - ptr->plockwait;
		highest_prio = q[my_ptr].qkey;
	}
	else{
		highest_prio = 0;
	}
	my_ptr = q[lptr->lqtail].qprev;
	ptr = &proctab[my_ptr];
	while(my_ptr != lptr->lqhead){
		if(ptr->plocktype == READ){
			if(q[my_ptr].qkey == highest_prio){
				if((ctr1000 - ptr->plockwait) - writer_wait <= 500){
					temp_ptr = my_ptr;
					my_ptr = q[my_ptr].qprev;
					ptr = &proctab[my_ptr];
					make_ready(lock,temp_ptr,READ);
				}
			}	
			else if(q[my_ptr].qkey > highest_prio){
				temp_ptr = my_ptr;
				my_ptr = q[my_ptr].qprev;
				ptr = &proctab[my_ptr];
				make_ready(lock,temp_ptr,READ);
			}
			else{
				my_ptr = q[my_ptr].qprev;
                		ptr = &proctab[my_ptr];
			}
		}
		else{
		my_ptr = q[my_ptr].qprev;
		ptr = &proctab[my_ptr];
		}
	}
}
//Function to put a proces in ready queue when selected
LOCAL void make_ready(int lock,int pid,int type){
	dequeue(pid);
	ready(pid,RESCHNO);
	if(type == READ){
		locks[lock].lreaders++;
		locks[lock].ltype = READ;
	}
	else{
		locks[lock].ltype = WRITE;
	}
	proctab[pid].lockid = -1;
        Priority_inherit(lock);
        Max_lprio(lock);
}
//Function to update pinh when releasing a lock
void update_pinh(int pid){
	int prio = 0,i;
	for(i=0;i<NLOCKS;i++){
		if(proctab[pid].lockheld[i] == READ || proctab[pid].lockheld[i] == WRITE){
			if(locks[i].lprio>prio){
				prio = locks[i].lprio;
			}
		}
	}
	proctab[pid].pinh = prio;
}
