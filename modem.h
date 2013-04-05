
#ifndef _MODEM_H_
#define _MODEM_H_

#include <Arduino.h>
#include "include.h"

struct modem;


/* Initializes the modem and a datastructure to keep track of it
 * Tries to connect for timeout seconds
 * Preconditions: A usable serial port,
 * 								A positive amount of time to check for the modem
 * Postconditions: A valid modem structure, or NULL if there was no modem attached
 */
struct modem *modemInit(USARTClass *serial, int timeout);

/* Frees the modem, disconnects the modem, puts it into a safe state.
 * Preconditions: A valid modem object
 * Postconditions: The modem object is now invalid
 */
void modemFree(struct modem *);


/* Checks for information from the modem, updates the state accordingly.
 * Preconditions: A valid modem object
 * Postconditions: The modem has up to date information about the state
 *								 of the hardware, and recieved any new information from
 *								 the other modem, if connected.
 */
void modemUpdate(struct modem *);

/* Basic modem queries */

/* Whether or not the modem has connected to another modem
 * Returns true if in a call with another modem
 * Preconditions: A valid modem object
 * Postconditions: The modem object is in the same state as before
 */
bool modemIsConn(struct modem *);

/* Whether or not the modem is even attached.
 * Precondtions: A valid modem object
 *							 A positive timeout
 * Postconditions: The modem object is in the same state as before
 */
bool modemCheckAttached(struct modem *, int timeout);

/* Returns a floating point value corresponding to the power to be sent
 * to the motors. In the range of -1.0 to 1.0; where 1.0 is the maximum
 * power, 0.0 is no power, and -1.0 is reverse.
 * These values are currently obtained from packets corresponding to
 * Santa Clara's format, but eventually this functionality will be removed
 * and put in a more suitable location.
 * Preconditions: A valid modem object, which has recieved a packet
 * Postconditions: The modem object is in the same state as before
 */
float modemForwardPwr(struct modem *modem);
float modemRotationPwr(struct modem *modem);

/* Whether or not the modem has a packet.
 * False on initialization, true after the first command has been recieved
 * Preconditions: A valid modem object
 * Postconditions: The modem object is in the same state as before
 */
bool modemHasPacket(struct modem *);

/* Whether or not the modem needs a packet to be sent back to the base
 * This is true if the kayak has recieved a command packet,
 * but has not replied with any new information
 * Preconditions: A valid modem object
 * Postconditions: The modem object is in the same state as before
 */
bool modemNeedsPacket(struct modem *);

/* Sends a packet back to the base.
 * Preconditions: A valid modem object, which is connected
 * Postconditions: The modem object is in the same state as before
 */
void modemSendPacket(struct modem *, void *packet, size_t size);

#endif
