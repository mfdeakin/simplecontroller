
#ifndef _COMPASS_H_
#define _COMPASS_H_

#include <Wire/Wire.h>

#include "include.h"

/* Returns the bearing of the compass relative to magnetic north
 * Preconditions: The compass wire
 */
float compassBearing();

#endif
