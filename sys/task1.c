#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>
#include <lock.h>
#include <sem.h>
#define DEFAULT_LOCK_PRIO 20

#define assert(x,error) if(!(x)){ \
            kprintf(error);\
            return;\
            }
int mystrncmp(char* des,char* target,int n){
    int i;
    for (i=0;i<n;i++){
        if (target[i] == '.') continue;
        if (des[i] != target[i]) return 1;
    }
    return 0;
}


void xinu_sem (char *msg, int sem)
{
        int     ret;

        kprintf ("  %s: to acquire lock\n", msg);
        wait(sem);
        kprintf ("  %s: acquired lock\n", msg);
	sleep(4);
        kprintf ("  %s: to release lock\n", msg);
        signal(sem);
}


void reader3 (char *msg, int lck)
{
        int     ret;

        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, READ, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock\n", msg);
	sleep(4);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void writer3 (char *msg, int lck)
{
        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, WRITE, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock, sleep 10s\n", msg);
        sleep (4);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void test3 ()
{
        int     lck;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nTest 3: test the basic priority inheritence\n");
        lck  = lcreate ();
        assert (lck != SYSERR, "Test 3 failed");

        rd1 = create(reader3, 2000, 25, "reader", 2, "reader A", lck);
        rd2 = create(reader3, 2000, 30, "reader", 2, "reader B", lck);
        wr1 = create(writer3, 2000, 20, "writerA", 2, "writer", lck);

        resume(wr1);
        sleep (1);

        resume(rd1);
        sleep (1);

        resume (rd2);
	sleep (1);
		
        sleep (15);
        kprintf ("Test 3 OK\n");
}
void test4 ()
{
        int     sem;
        int     sem_rd1, sem_rd2;
        int     sem_wr1;

        kprintf("\nTest 4: Xinu Semaphore implementation\n");
        sem  = screate (1);
        assert (sem != SYSERR, "Test 3 failed");

        sem_rd1 = create(xinu_sem, 2000, 25, "reader", 2, "reader A", sem);
        sem_rd2 = create(xinu_sem, 2000, 30, "reader", 2, "reader B", sem);
        sem_wr1 = create(xinu_sem, 2000, 20, "writer", 2, "writer", sem);

        resume(sem_wr1);
        sleep (1);

        resume(sem_rd1);
        sleep (1);
		
        resume (sem_rd2);
	sleep (1);

        sleep (15);
        kprintf ("Test 4 OK\n");
}


int main( )
{
        /* These test cases are only used for test purpose.
         * The provided results do not guarantee your correctness.
         * You need to read the PA2 instruction carefully.
         */
		test3();
		test4();

        /* The hook to shutdown QEMU for process-like execution of XINU.
         * This API call exists the QEMU process.
         */
        return 0;
}
