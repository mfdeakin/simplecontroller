
#ifndef _INCLUDE_H_
#define _INCLUDE_H_

/* A measurement of the SAM3X's number of ticks per 10 ms
 * Used with GetTickCount() function
 * Basically, the SAM3X CMSIS software starts a timer which goes of 16 times per 10 ms
 * It then uses this to increment the number of ticks each time the timer fires
 * This number is retrieved by GetTickCount
 * If I knew where in the code this interrupt was specified, I'd definitely just
 * look for the number of interrupts per second there
 */
#define TICKSPERTENMS 16

#define DEBUGSERIAL Serial
#define GPSSERIAL Serial1
#define MOTORSERIAL Serial3
#define MODEMSERIAL Serial2

#ifndef RELEASE_VERSION
#define DEBUGPRINT(str) DEBUGSERIAL.print(str)
#define DEBUGPRINTHEX(val) DEBUGSERIAL.print(val, HEX)
#else
#define DEBUGPRINT(str) {}
#define DEBUGPRINTHEX(val) {}
#endif

#endif
