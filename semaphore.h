
#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* semTryUp tries to increment the semaphore
 * It returns true if it succeeds, otherwise false
 * Preconditions: sem is a valid pointer to an integer
 * Postconditions: sem has been incremented in an exclusive manner,
 * 								 or not modified at all
 */
int semTryUp(int *sem);

/* semUp busy loops until it increments the semaphore
 * This is the function to use for semaphores
 * Preconditions: sem is a valid pointer to an integer
 * Postconditions: sem has been incremented in an exclusive manner
 */
void semUp(int *sem);

/* semTryDown tries to decrement the semaphore
 * It returns 1 if it succeeds, -1 if it can't exclusively access the semaphore,
 * and 0 if the semaphore is already locked.
 * Preconditions: sem is a valid pointer to an integer
 * Postconditions: sem has been decremented in an exclusive manner,
 * 								 or not modified at all
 */
int semTryDown(int *sem);

/* semDown busy loops to decrement the semaphore
 * Don't use this. A real multitasking environment is necessary for
 * it to work as expected. Currently it is just a spin lock, which is bad,
 * especially in a non-premptive environment (like this)
 * Preconditions: sem is a valid pointer to an integer
 * Postconditions: sem has been decremented in an exclusive manner,
 */
void semDown(int *sem);

#ifdef __cplusplus
};
#endif

#endif
