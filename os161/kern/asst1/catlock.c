/*
 * catlock.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use LOCKS/CV'S to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

struct lock* lock1;	//locks the first food bowl
struct lock* lock2;	//locks the second food bowl
struct lock* lock3;	//lock for changing counter
struct cv* cv1;		//Cat can eat
struct cv* cv2;		//Mouse can eat
int counter=0;		//Counter=cats eating - mouse eating
/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */
static void
lock_eat(const char *who, int num, int bowl, int iteration)
{
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        clocksleep(1);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
}

/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
	int iteration=0;
	//int spl;
	while(iteration<4){
		if(lock1->count==0){
			lock_acquire(lock1);
			while(counter <0){
				cv_wait(cv1,lock1);
			}
			lock_acquire(lock3);
			counter++;
			lock_release(lock3);
			lock_eat("cat",catnumber,1,iteration);
			iteration++;

			lock_acquire(lock3);
			counter--;
			lock_release(lock3);
			lock_release(lock1);
			cv_broadcast(cv2,lock1);
		}
		else if(lock2->count==0){
			lock_acquire(lock2);
			while(counter <0){
				cv_wait(cv1,lock2);
			}
			lock_acquire(lock3);
			counter++;
			lock_release(lock3);
			assert(lock_do_i_hold(lock2));
			lock_eat("cat",catnumber,2,iteration);
			iteration++;
			lock_acquire(lock3);
			counter--;
			lock_release(lock3);
			lock_release(lock2);
			cv_broadcast(cv2,lock2);
		}
        (void) unusedpointer;
}
	
}
/*
 * mouselock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *tlock(void * unusedpointer,
        unsigned long catnum
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
        int iteration=0;
//	int spl;
	while(iteration<4){
		if(lock1->count==0){
			lock_acquire(lock1);
			while(counter >0){
				cv_wait(cv2,lock1);
			}
			lock_acquire(lock3);
			counter--;
			lock_release(lock3);
			assert(lock_do_i_hold(lock1));
			lock_eat("mouse",mousenumber,1,iteration);
			iteration++;
			lock_acquire(lock3);
			counter++;
			lock_release(lock3);
			lock_release(lock1);
			cv_broadcast(cv1,lock1);
		}
		else if(lock2->count==0){
			lock_acquire(lock2);
			while(counter >0){
				cv_wait(cv2,lock2);
			}
			lock_acquire(lock3);
			counter--;
			lock_release(lock3);
			assert(lock_do_i_hold(lock2));
			lock_eat("mouse",mousenumber,2,iteration);
			iteration++;
			lock_acquire(lock3);
			counter++;
			lock_release(lock3);
			lock_release(lock2);
			cv_broadcast(cv1,lock2);
		}
        (void) unusedpointer;
	}

}
/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
	lock1 =lock_create("bowl1");
	lock2 =lock_create("bowl2");
	lock3 =lock_create("counter");
	cv1 = cv_create("catcaneat");
	cv2 = cv_create("mousecaneat");
	/*
         * Start NCATS catlock() threads.
         */
        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */
                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        return 0;
}

/*
 * End of catlock.c
 */
