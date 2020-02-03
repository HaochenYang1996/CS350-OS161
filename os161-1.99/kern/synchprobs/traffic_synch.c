

#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
//haochen
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

static int volatile waitList[12]; 

static struct lock *trafficLock;
static struct cv *ns;   
static struct cv *ne;
static struct cv *nw;

static struct cv *sn;
static struct cv *se;
static struct cv *sw;

static struct cv *en;
static struct cv *es;
static struct cv *ew;

static struct cv *wn;
static struct cv *ws;
static struct cv *we;

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
	ns = cv_create("ns"); 
	ne = cv_create("ne");
	nw = cv_create("nw");

	sn = cv_create("sn");
	se = cv_create("se");
	sw = cv_create("sw");

	en = cv_create("en");
	es = cv_create("es");
	ew = cv_create("ew");

	wn = cv_create("wn");
	ws = cv_create("ws");
	we = cv_create("we");


	// chekc null
	if (trafficLock == NULL) {
    panic("could not create Traffic lock ");
	}
	if (ns == NULL  ) {
		panic("could not create ns CV");
	}
	if (ne == NULL) {
		panic("could not create ns CV");
	}
	if (nw == NULL) {
		panic("could not create nw CV");
	}

	if (sn == NULL) {
		panic("could not create sn CV");
	}
	if (se == NULL) {
		panic("could not create se CV");
	}
	if (sw == NULL) {
		panic("could not create sw CV");
	}

	if (en == NULL) {
		panic("could not create en CV");
	}
	if (es == NULL) {
		panic("could not create es CV");
	}
	if (ew == NULL) {
		panic("could not create ew CV");
	}

	if (wn == NULL) {
		panic("could not create wn CV");
	}
	if (ws == NULL) {
		panic("could not create ws CV");
	}
	if (we == NULL) {
		panic("could not create we CV");
	}
	waitList[0] = 0;  //ns
	waitList[1] = 0;  //ne
	waitList[2] = 0;  //nw
	waitList[3] = 0;  //sn
	waitList[4] = 0;  //se
	waitList[5] = 0;  //sw
	waitList[6] = 0;  //en
	waitList[7] = 0;  //es
	waitList[8] = 0;  //ew
	waitList[9] = 0;  //wn
	waitList[10] = 0; //ws
	waitList[11] = 0; //we
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

	KASSERT(ns != NULL);  
	KASSERT(ne != NULL);
	KASSERT(nw != NULL);

	KASSERT(sn != NULL);
	KASSERT(se != NULL);
	KASSERT(sw != NULL);

	KASSERT(en != NULL);
	KASSERT(es != NULL);
	KASSERT(ew != NULL);

	KASSERT(wn != NULL);
	KASSERT(ws != NULL);
	KASSERT(we != NULL);

	cv_destroy(ns);
	cv_destroy(ne);
	cv_destroy(nw);

	cv_destroy(se);
	cv_destroy(sw);
	cv_destroy(sn);

	cv_destroy(es);
	cv_destroy(en);
	cv_destroy(ew);

	cv_destroy(ws);
	cv_destroy(we);
	cv_destroy(wn);
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

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
//   (void)origin;  /* avoid compiler complaint about unused parameter */
//   (void)destination; /* avoid compiler complaint about unused parameter */
  // KASSERT(intersectionSem != NULL);
  // P(intersectionSem);
  //when acquire northCV, means we can go from north to our destination
	KASSERT(ns != NULL);  
	KASSERT(ne != NULL);
	KASSERT(nw != NULL);

	KASSERT(sn != NULL);
	KASSERT(se != NULL);
	KASSERT(sw != NULL);

	KASSERT(en != NULL);
	KASSERT(es != NULL);
	KASSERT(ew != NULL);

	KASSERT(wn != NULL);
	KASSERT(ws != NULL);
	KASSERT(we != NULL);
	KASSERT(trafficLock != NULL);
	switch (origin){
		case north:
			// north -> south
			if (destination == south) {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[8] != 0 || waitList[11] != 0 || waitList[7] != 0 || waitList[10] != 0 || waitList[9] != 0 || waitList[5] != 0)
					{
						cv_wait(ns, trafficLock);
					} else {
						waitList[0] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			// north -> east
			else if (destination == east) {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[4] != 0 || waitList[11] != 0 || waitList[5] != 0 || waitList[3] != 0 || waitList[7] != 0 || waitList[9] != 0 || waitList[8] != 0)
					{     
						cv_wait(ne, trafficLock);
					} else {
						waitList[1] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			else {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[8] != 0 || waitList[5] != 0) 
					{
            cv_wait(nw, trafficLock);
					} else {
						waitList[2] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			break;
		case south:
			// south -> north
			if (destination == north) {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[11] != 0 || waitList[8] != 0 || waitList[9] != 0 || waitList[6] != 0 || waitList[7] != 0 || waitList[1] != 0)
					{
						cv_wait(sn, trafficLock);
					} else {
						waitList[3] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			// south -> west
			else if (destination == west) {
				lock_acquire(trafficLock);
				while (1){
					if (waitList[8] != 0 || waitList[2] != 0 || waitList[11] != 0 || waitList[0] != 0 || waitList[7] != 0 || waitList[1] != 0 || waitList[9] != 0)
					{
            cv_wait(sw, trafficLock);
					} else {
						waitList[5] += 1;
						break;
					}
			}
			lock_release(trafficLock);
			}
			// south -> east, right turn 
			else {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[11] != 0 || waitList[1] != 0) 
					{
            cv_wait(se, trafficLock);
					} else {
						waitList[4] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			break;
		case west:
			// west -> east
			if (destination == east) {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[0] != 0 || waitList[3] != 0 || waitList[1] != 0 || waitList[4] != 0 || waitList[7] != 0 || waitList[5] != 0)
					{
						cv_wait(we, trafficLock);
					} else {
						waitList[11] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}

			// west->north
			else if (destination == north) {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[3] != 0 || waitList[6] != 0 || waitList[7] != 0 || waitList[1] != 0 || waitList[5] != 0 || waitList[8] != 0 || waitList[0] != 0) 
					{
						cv_wait(wn, trafficLock);
					} else {
						waitList[9] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			// west->south
			else {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[0] != 0 || waitList[7] != 0) 
					{        
						cv_wait(ws, trafficLock);
					} else {
						waitList[10] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			break;
		case east:
			// east->south
			if (destination == south) {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[0] != 0 || waitList[10] != 0 || waitList[1] != 0 || waitList[11] != 0 || waitList[9] != 0 || waitList[5] != 0 || waitList[3] != 0) 
					{
						cv_wait(es, trafficLock);
					} else {
						waitList[7] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}

			// east -> west
			//ns , sn, nw, sw, ne, wn
			else if (destination == west) {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[2] != 0 || waitList[0] != 0 || waitList[3] != 0 || waitList[9] != 0 || waitList[1] != 0 || waitList[5] != 0) 
					{          
						cv_wait(ew, trafficLock);
					} else {
						waitList[8] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			// east -> north
			else {
				lock_acquire(trafficLock);
				while (1) {
					if (waitList[3] != 0 || waitList[9] != 0) 
					{      
						cv_wait(en, trafficLock);
					} else {
						waitList[6] += 1;
						break;
					}
				}
				lock_release(trafficLock);
			}
			break;
	}
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

void
intersection_after_exit(Direction origin, Direction destination) 
{
	KASSERT(ns != NULL);  
	KASSERT(ne != NULL);
	KASSERT(nw != NULL);

	KASSERT(sn != NULL);
	KASSERT(se != NULL);
	KASSERT(sw != NULL);

	KASSERT(en != NULL);
	KASSERT(es != NULL);
	KASSERT(ew != NULL);

	KASSERT(wn != NULL);
	KASSERT(ws != NULL);
	KASSERT(we != NULL);
	KASSERT(trafficLock != NULL);
	lock_acquire(trafficLock);
	switch (origin) {
	case north:
		if (destination == east) {
			// ne
			waitList[1]-=1;
			cv_broadcast(es, trafficLock);
			cv_broadcast(ew, trafficLock);
			cv_broadcast(sn, trafficLock);
			cv_broadcast(sw, trafficLock);
			cv_broadcast(se, trafficLock);
			cv_broadcast(we, trafficLock);
			cv_broadcast(wn, trafficLock);
		} else if (destination == south) { 
			// ns
			waitList[0]-=1;
			cv_broadcast(es, trafficLock);
			cv_broadcast(ew, trafficLock);
			cv_broadcast(sw, trafficLock);
			cv_broadcast(we, trafficLock);
			cv_broadcast(wn, trafficLock);
			cv_broadcast(ws, trafficLock);
		} else {
			//nw
			waitList[2]-=1;
			cv_broadcast(ew, trafficLock);
			cv_broadcast(sw, trafficLock);
		}
		break;
	case east:
			if (destination == west) { 
				//ew
				waitList[8]-=1;
				cv_broadcast(ns, trafficLock);
				cv_broadcast(ne, trafficLock);
				cv_broadcast(nw, trafficLock);
				cv_broadcast(sn, trafficLock);
				cv_broadcast(sw, trafficLock);
				cv_broadcast(wn, trafficLock);
			} else if (destination == south) { 
				//es
				waitList[7] -=1;
				cv_broadcast(ns, trafficLock);
				cv_broadcast(ne, trafficLock);
				cv_broadcast(sn, trafficLock);
				cv_broadcast(sw, trafficLock);
				cv_broadcast(we, trafficLock);
				cv_broadcast(wn, trafficLock);
				cv_broadcast(ws, trafficLock);
			} else { 
				//en
				waitList[6] -=1;
				cv_broadcast(sn, trafficLock);
				cv_broadcast(wn, trafficLock);
			}
		break;
	case south:
		if (destination == east) {
			//se
			waitList[4]-=1;
			cv_broadcast(ne, trafficLock);
			cv_broadcast(we, trafficLock);
		} else if (destination == west) { 
			//sw
			waitList[5] -=1;
			cv_broadcast(ns, trafficLock);
			cv_broadcast(ne, trafficLock);
			cv_broadcast(nw, trafficLock);
			cv_broadcast(es, trafficLock);
			cv_broadcast(ew, trafficLock);
			cv_broadcast(we, trafficLock);
			cv_broadcast(wn, trafficLock);
		} else {
			//sn
			waitList[3] -=1;
			cv_broadcast(ne, trafficLock);
			cv_broadcast(es, trafficLock);
			cv_broadcast(ew, trafficLock);
			cv_broadcast(en, trafficLock);
			cv_broadcast(we, trafficLock);
			cv_broadcast(wn, trafficLock);
		}
		break;
	case west:
		if (destination == east) { 
			//we
			waitList[11] -=1;
			cv_broadcast(ns, trafficLock);
			cv_broadcast(ne, trafficLock);
			cv_broadcast(es, trafficLock);
			cv_broadcast(sn, trafficLock);
			cv_broadcast(sw, trafficLock);
			cv_broadcast(se, trafficLock);
		} else if (destination == south) { 
			// ws
			waitList[10] -=1;
			cv_broadcast(ns, trafficLock);
			cv_broadcast(es, trafficLock);
		} else { 
			//wn
			waitList[9] -=1;
			cv_broadcast(ns, trafficLock);
			cv_broadcast(ne, trafficLock);
			cv_broadcast(es, trafficLock);
			cv_broadcast(ew, trafficLock);
			cv_broadcast(en, trafficLock);
			cv_broadcast(sn, trafficLock);
			cv_broadcast(sw, trafficLock);
		}
		break;
	}
	lock_release(trafficLock);
    return;
}
