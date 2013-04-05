
#ifndef _MOTOR_H_
#define _MOTOR_H_

#define RELEASE_VERSION

#include <Arduino.h>
#include "include.h"

struct motorctrl;

/* Structure corresponding to the motor controllers channels
 * cA -> Channel A
 * cB -> Channel B
 */
struct channelpair {
	int cA, cB;
};

/* Initializes the motor controller object connected to the serial port
 * If no motor controller is detected in the required timeout, returns NULL.
 * Preconditions: A valid serial port, a positive timeout
 * Postconditions: The motor controller is detected within the timeout and
 *                 then initialized.
 */
struct motorctrl *motorInit(USARTClass *serial, int timeout);

/* Frees the motor controller, turns off the motors
 * Preconditions: A valid motor controller object
 * Postconditions: The motor controller object memory is released,
 *                 The motor controllers motors are turned off
 */
void motorFree(struct motorctrl *);

/* Writes a command to the motor controller, and waits for a response.
 * Returns the number of bytes sent in response to the command
 * Preconditions: A valid motor controller object, a valid null terminated
 *                command string, a valid buffer of size size,
 *								and a positive timeout.
 * Postconditions: The command is written to the motor controller, and the
 *                 response of the motor controller is returned within the.
 *                 timeout specified.
 *                 Any information buffered before the command is sent is
 *                 discarded. Any information that is sent after the specified
 *                 amount of data is recieved is also discarded.
 */
int motorWriteCmd(struct motorctrl *, const char *cmd,
									 void *buffer, size_t size, int timeout);

/* Sets the speeds of the motors
 * Uses a differential controller
 * Preconditions: A valid motor object, floating point values between -1 and 1
 *								1 is full power forward, 0 is off, -1 is full power reverse
 * Postconditions: The motor controller powers the motors at the percent
 *                 specified.
 */
void motorSetSpeed(struct motorctrl *, float forward, float rotate);

/* Reads the number of amps that the motor controller is providing
 * to the motors. Returns in a channel pair struct, corresponding to
 * each motor.
 * Preconditons: A valid motor object.
 * Postconditions: The state of the motor controller is preserved,
 *                 The number of amps being sent to each motor is updated
 */
struct channelpair motorCheckAmp(struct motorctrl *);

/* Reads the average number of watts that the motor controller is providing
 * to the motors. Returns in a channel pair struct, corresponding to
 * each motor.
 * Preconditons: A valid motor object.
 * Postconditions: The state of the motor controller is preserved,
 *                 The number of watts being sent to each motor is updated
 */
struct channelpair motorCheckWatt(struct motorctrl *);

#endif
