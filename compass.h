
#ifndef _COMPASS_H_
#define _COMPASS_H_

#include <Wire/Wire.h>

#include "include.h"

struct compass;

/* Returns a valid compass structure on success, NULL on failure
 * Preconditions: The scheduler is initialized
 * Postconditions: An event to periodically update the compass bearing is 
 *								 registered, a valid compass object is returned
 */
struct compass *compassInit(TwoWire *wire);

/* Returns the bearing of the compass relative to magnetic north
 * Preconditions: A valid compass object
 * Postconditions: The compass objects state remains the same,
 * 								 the bearing is returned
 */
float compassBearing(struct compass *);

#endif
