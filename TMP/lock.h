#ifndef _LOCK_H_
#define _LOCK_H_

#define NLOCKS 50
#define READ 2
#define WRITE 1
#define NONE 0
#define LFREE   '\01'           /* this lock is free               */
#define LUSED   '\02'           /* this lock is used               */

struct  lentry  {               /* lock table entry                */
        char    lstate;         /* the state SFREE or SUSED             */
        int     lreaders;       /* count for readers            */
        int     lqhead;         /* q index of head of list              */
        int     lqtail;         /* q index of tail of list              */
	int     ltype;          /* Type of lock */
	int     lprio;          /* Priority */
	int 	pidheld[NPROC];
};
extern  struct  lentry  locks[];

#define isbadlock(s)     (s<0 || s>=NLOCKS)
extern unsigned long ctr1000;
extern abs(int arg);

extern void linit(void);
extern int  lcreate(void);
extern int  ldelete(int lockdescriptor);
extern int  lock(int ldes1, int type, int priority);
extern int  releaseall(int numlocks, long ldes1, ...);

#endif
