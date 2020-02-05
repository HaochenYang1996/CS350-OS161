

#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
//haochen3
/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
//  */
// static struct semaphore *intersectionSem;

static int fourDirStatus[4];
static int waitList[4]; 
static int dir = -1;  

static struct lock *trafficLock;
static struct cv *n;   
static struct cv *s;
static struct cv *w;
static struct cv *e;

/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

  // intersectionSem = sem_create("intersectionSem",1);
  // if (intersectionSem == NULL) {
  //   panic("could not create intersection semaphore");
  // }
  // return;

    trafficLock = lock_create("trafficLock");
    n = cv_create("n"); 
    s = cv_create("s");
    w = cv_create("w");
    e = cv_create("e");

    // chekc null
    if (trafficLock == NULL) {
    panic("could not create Traffic lock ");
    }
    if (n == NULL  ) {
        panic("could not create n CV");
    }
    if (s == NULL) {
        panic("could not create s CV");
    }
    if (w == NULL) {
        panic("could not create w CV");
    }

    if (e == NULL) {
        panic("could not create e CV");
    }
    waitList[0] = 0;
    waitList[1] = 0;
    waitList[2] = 0;
    waitList[3] = 0;
    return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  // KASSERT(intersectionSem != NULL);
  // sem_destroy(intersectionSem);

    KASSERT(trafficLock != NULL);
    lock_destroy(trafficLock);

    KASSERT(n != NULL);  
    KASSERT(s != NULL);
    KASSERT(w != NULL);
    KASSERT(e != NULL);
    cv_destroy(n);
    cv_destroy(s);
    cv_destroy(w);
    cv_destroy(e);
    return;
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */


// idea of intersection_before_entry follows lecture notes product/consume example
// here is the psuedo code:
// acqurire traffic lock first:
// if (intersection is being used by other vehicles' moves which block current move):
//      wait on its cv: wait for the current path till it's avaialbe
// else:
//      waitList[current Path] += 1 (means one more car is waiting on this cv)
// release traffic lock

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
//   (void)origin;  /* avoid compiler complaint about unused parameter */
    (void)destination; /* avoid compiler complaint about unused parameter */
  // KASSERT(intersectionSem != NULL);
  // P(intersectionSem);
  //when acquire northCV, means we can go from north to our destination
    KASSERT(trafficLock != NULL);
    KASSERT(n != NULL);  
    KASSERT(s != NULL);
    KASSERT(w != NULL);
    KASSERT(e != NULL);
    lock_acquire(trafficLock);
    switch (origin){
        case north:
            if ((dir!= 0 && dir!=-1) || fourDirStatus[1]!=0|| fourDirStatus[2]!=0|| fourDirStatus[3]!=0) {
                waitList[0]+=1;
                cv_wait(n, trafficLock);
            }
            if (dir == -1) {
                dir = 0;
            }
            fourDirStatus[0]+=1;
            waitList[0]=0;
            break;
        case east:
            if ((dir!= 1 && dir!=-1) || fourDirStatus[0]!=0|| fourDirStatus[2]!=0|| fourDirStatus[3]!=0) {
                waitList[1]+=1;
                cv_wait(e, trafficLock);
            }
            if (dir == -1) {
                dir = 1;
            }
            fourDirStatus[1]+=1;
            waitList[1]=0;
            break;
        case south:
            if ((dir!= 2 && dir!=-1) || fourDirStatus[0]!=0|| fourDirStatus[1]!=0|| fourDirStatus[3]!=0) {
                waitList[2]+=1;
                cv_wait(s, trafficLock);
            }
            if (dir == -1) {
                dir = 2;
            }
            fourDirStatus[2]+=1;
            waitList[2]=0;
            break;
        case west:
            if ((dir!= 3 && dir!=-1) || fourDirStatus[0]!=0|| fourDirStatus[1]!=0|| fourDirStatus[2]!=0) {
                waitList[3]+=1;
                cv_wait(w, trafficLock);
            }
            if (dir == -1) {
                dir = 3;
            }
            fourDirStatus[3]+=1;
            waitList[3]=0;
            break;
    }

    lock_release(trafficLock);
    return;
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

// idea of intersection_after_exit follows lecture notes product/consume example
// when a car leaves its intersection:
// lock traffic lock
// waitList[current Path] += 1 (means one less car is waiting on this cv)
// broadcasr all other CVs which were blocked.
// release traffic lock

// n e s w
// 0 1 2 3
void
intersection_after_exit(Direction origin, Direction destination) 
{
    (void) destination;
    KASSERT(trafficLock != NULL);
    KASSERT(n != NULL);  
    KASSERT(s != NULL);
    KASSERT(w != NULL);
    KASSERT(e != NULL);
    lock_acquire(trafficLock);
    switch (origin) {
    case north:
        fourDirStatus[0]-=1;
        break;
    case east:
        fourDirStatus[1]-=1;
        break;
    case south:
        fourDirStatus[2]-=1;
        break;
    case west:
        fourDirStatus[3]-=1;
        break;
    }
    if (fourDirStatus[0]==0 && fourDirStatus[1]==0 && fourDirStatus[2]==0 && fourDirStatus[3]==0) {
        if (waitList[0]==0 && waitList[1]==0 && waitList[2]==0 && waitList[3]==0) {
            dir = -1;
            lock_release(trafficLock);
            return;
        } else {
                switch (origin) {
                    case north:
                        if (waitList[3]>0) {
                            dir = 3;
                            cv_broadcast(w, trafficLock);
                        } else if (waitList[2]>0) {
                            dir = 2;
                            cv_broadcast(s, trafficLock);
                        } else {
                            dir = 1;
                            cv_broadcast(e, trafficLock);
                        }
                        break;
                    case east:
                        if (waitList[0]>0) {
                            dir = 0;
                            cv_broadcast(n, trafficLock);
                        } else if (waitList[3]>0) {
                            dir = 3;
                            cv_broadcast(w, trafficLock);
                        } else {
                            dir = 2;
                            cv_broadcast(s, trafficLock);
                        }
                        break;
                    case south:
                        if (waitList[1]>0) {
                            dir = 1;
                            cv_broadcast(e, trafficLock);
                        } else if (waitList[0]>0) {
                            dir = 0;
                            cv_broadcast(n, trafficLock);
                        } else {
                            dir = 3;
                            cv_broadcast(w, trafficLock);
                        }
                        break;
                    case west:
                        if (waitList[2]>0) {
                            dir = 2;
                            cv_broadcast(s, trafficLock);
                        } else if (waitList[1]>0) {
                            dir = 1;
                            cv_broadcast(e, trafficLock);
                        } else {
                            dir = 0;
                            cv_broadcast(n, trafficLock);
                        }
                        break;
                } 
        }
    }
    lock_release(trafficLock);
    return;
}
