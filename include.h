
#ifndef _INCLUDE_H_
#define _INCLUDE_H_

/* A measurement of the SAM3X's number of ticks per 10 ms
 * Used with GetTickCount() function
 * Basically, the SAM3X CMSIS software starts a timer which goes
 * off 16 times per 10 ms
 * It then uses this to increment the number of ticks each time the timer fires
 * This number is retrieved by GetTickCount
 * If I knew where in the code this interrupt was specified, I'd definitely just
 * look for the number of interrupts per second there
 */
#define TICKSPERTENMS 16

/* Functions of the serial ports */
#define DEBUGSERIAL Serial
#define GPSSERIAL Serial1
#define MOTORSERIAL Serial3
#define MODEMSERIAL Serial2

#define COMPASSWIRE Wire

#define INT_MAX 0x7FFFFFFF
#define UINT_MAX 0xFFFFFFFF

/* Macros for easy switching to and from a debug and release version */
#ifndef RELEASE_VERSION
#define DEBUGPRINT(str) DEBUGSERIAL.print(str)
#define DEBUGPRINTHEX(val) DEBUGSERIAL.print(val, HEX)
#else
#define DEBUGPRINT(str) {}
#define DEBUGPRINTHEX(val) {}
#endif

#endif
