/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
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
//#include <machine/spl.h>

//int spl = splhigh();  //disable
//splx(spl);  //enable


/*
 *
 * Constants
 *
 */

/*
 * Number of cars created.
 */

#define NCARS 20

int endSemaNum=0;
/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

//create for semaphores for all 4 regions
struct semaphore *NE;
struct semaphore *SE;
struct semaphore *SW;
struct semaphore *NW;
struct semaphore *Slock;  //use as an intersection lock, allowing max 3 cars in intersection

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        
        (void) cardirection;
        (void) carnumber;

	//car is coming from North
	if(cardirection == 0){
		//P(Slock);//lock
		message(APPROACHING, carnumber, cardirection, 2);
		P(NW); //region 1
		message(REGION1, carnumber, cardirection, 2);
		//V(Slock);//unlock

		P(SW);
		message(REGION2, carnumber, cardirection, 2); //2 is south

		V(NW);
		message(LEAVING, carnumber, cardirection, 2); //2 is south
		V(SW);
	}
	//car is coming from East
	else if(cardirection == 1){
		//P(Slock);//lock
		message(APPROACHING, carnumber, cardirection, 3);
		P(NE); //region 1
		message(REGION1, carnumber, cardirection, 3);
		//V(Slock);//unlock

		P(NW);
		message(REGION2, carnumber, cardirection, 3); //3 is west

		V(NE);
		message(LEAVING, carnumber, cardirection, 3); //3 is west
		V(NW);
	}
	//car is coming from South
	else if(cardirection == 2){
		//P(Slock);//lock
		message(APPROACHING, carnumber, cardirection, 0);
		P(SE); //region 1
		message(REGION1, carnumber, cardirection, 0);
		//V(Slock);//unlock

		P(NE);
		message(REGION2, carnumber, cardirection, 0); //0 is north

		V(SE);
		message(LEAVING, carnumber, cardirection, 0); //0 is north
		V(NE);
	}
	//car is coming from West
	else if(cardirection == 3){
		//P(Slock);//lock
		message(APPROACHING, carnumber, cardirection, 1);
		P(SW); //region 1
		message(REGION1, carnumber, cardirection, 1);
		//V(Slock);//unlock

		P(SE);
		message(REGION2, carnumber, cardirection, 1); //1 is east

		V(SW);
		message(LEAVING, carnumber, cardirection, 1); //1 is east
		V(SE);
	}

}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;

	//car is coming from North
	if(cardirection == 0){
		//P(Slock);//lock
		message(APPROACHING, carnumber, cardirection, 1);
		P(NW); //region 1
		message(REGION1, carnumber, cardirection, 1);
		//V(Slock);//unlock

		P(SW);
		message(REGION2, carnumber, cardirection, 1); //1 is east

		V(NW);

		P(SE);
		message(REGION3, carnumber, cardirection, 1); //1 is east
		V(SW);
		message(LEAVING, carnumber, cardirection, 1); //1 is east
		V(SE);
	}
	//car is coming from East
	else if(cardirection == 1){
		//P(Slock);//lock
		message(APPROACHING, carnumber, cardirection, 2);
		P(NE); //region 1
		message(REGION1, carnumber, cardirection, 2);
		//V(Slock);//unlock

		P(NW);
		message(REGION2, carnumber, cardirection, 2); //2 is south

		V(NE);

		P(SW);
		message(REGION3, carnumber, cardirection, 2); //2 is south
		V(NW);
		message(LEAVING, carnumber, cardirection, 2); //2 is south
		V(SW);


	}
	//car is coming from South
	else if(cardirection == 2){
		//P(Slock);//lock
		message(APPROACHING, carnumber, cardirection, 3);
		P(SE); //region 1
		message(REGION1, carnumber, cardirection, 3);
		//V(Slock);//unlock

		P(NE);
		message(REGION2, carnumber, cardirection, 3); //3 is west

		V(SE);

		P(NW);
		message(REGION3, carnumber, cardirection, 3); //3 is west
		V(NE);
		message(LEAVING, carnumber, cardirection, 3); //3 is west
		V(NW);
	}
	//car is coming from West
	else if(cardirection == 3){
		//P(Slock);//lock
		message(APPROACHING, carnumber, cardirection, 0);
		P(SW); //region 1
		message(REGION1, carnumber, cardirection, 0);
		//V(Slock);//unlock

		P(SE);
		message(REGION2, carnumber, cardirection, 0); //0 is north

		V(SW);

		P(NE);
		message(REGION3, carnumber, cardirection, 0); //0 is north
		V(SE);
		message(LEAVING, carnumber, cardirection, 0); //0 is north
		V(NE);
	}

}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;

	//car is coming from North
	if(cardirection == 0){
		//P(Slock);
		message(APPROACHING, carnumber, cardirection, 3);
		P(NW);
		message(REGION1, carnumber, cardirection, 3); //3 is west
		//V(Slock);

		message(LEAVING, carnumber, cardirection, 3); //3 is west
		V(NW);
	}
	//car is coming from East
	else if(cardirection == 1){
		//P(Slock);
		message(APPROACHING, carnumber, cardirection, 0);
		P(NE);
		message(REGION1, carnumber, cardirection, 0);
		//V(Slock);

		message(LEAVING, carnumber, cardirection, 0); //0 is north
		V(NE);
	}
	//car is coming from South
	else if(cardirection == 2){
		//P(Slock);
		message(APPROACHING, carnumber, cardirection, 1);
		P(SE);
		message(REGION1, carnumber, cardirection, 1);
		//V(Slock);

		message(LEAVING, carnumber, cardirection, 1); //1 is east
		V(SE);
	}
	//car is coming from West
	else if(cardirection == 3){
		//P(Slock);
		message(APPROACHING, carnumber, cardirection, 2);
		P(SW);
		message(REGION1, carnumber, cardirection, 2);
		//V(Slock);

		message(LEAVING, carnumber, cardirection, 2); //2 is south
		V(SW);
	}

}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection;

        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;
        (void) carnumber;
	(void) gostraight;
	(void) turnleft;
	(void) turnright;

        /*
         * cardirection is set randomly.
         */

	//cardirection = 0 ---> from North
	//cardirection = 1 ---> from East
	//cardirection = 2 ---> from South
	//cardirection = 3 ---> from West

        cardirection = random() % 4;

	//move = 0 ---> go straight
	//move = 1 ---> go right
	//move = 2 ---> go left

	int move = random() % 3;

	//lock the intersections if 3 cars are already inside
	P(Slock);
	if(move == 0)
		gostraight(cardirection, carnumber);
	else if(move == 1)
		turnright(cardirection, carnumber);
	else if(move == 2)
		turnleft(cardirection, carnumber);
	V(Slock);
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

	//initialize the semaphores
	NE = sem_create("NE",1);
	SE = sem_create("SE",1);
	SW = sem_create("SW",1);
	NW = sem_create("NW",1);

	Slock = sem_create("Slock",3); //maximum 3 cars at the intersections

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }
        return 0;
}
