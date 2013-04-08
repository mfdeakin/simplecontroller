
#include "semaphore.h"
#include <Arduino.h>
#include <core_cmInstr.h>

int semTryUp(int *sem)
{
	/* From:
	 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/BABDEEJC.html
	 */
	noInterrupts();
	int semval = __LDREXW(sem);
	semval++;
	int status = __STREXW(semval, sem);
	interrupts();
	/* In the future, should wake one thing that
	 * needs this semaphore after this
	 */
	return status == 0;
}

int semTryDown(int *sem)
{
	noInterrupts();
	int semval = __LDREXW(sem);
	int locked = 0;
	if(semval > 0) {
		semval--;
		locked = 1;
	}
	int status = __STREXW(semval, sem);
	interrupts();
	if(status == 0) {
		if(locked)
			return 1;
		return 0;
	}
	return -1;
}

void semUp(int *sem)
{
	int status = 0;
	do {
		status = semTryUp(sem);
	} while(!status);
}

void semDown(int *sem)
{
	/* This is the wrong way to do this!!!! */
	int status = 0;
	do {
		status = semTryDown(sem);
	} while(!status);
}
