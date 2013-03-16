
#ifndef _MODEM_H_
#define _MODEM_H_

#include <Arduino.h>
#include "include.h"
#include "ieeehalfprecision.h"

struct modem;

/* Initializes the modem and a datastructure to keep track of it
 * Tries to connect for timeout seconds
 */
struct modem *modemInit(USARTClass *serial, int timeout);
/* Frees the modem, disconnects the modem */
void modemFree(struct modem *);

bool modemIsConn(struct modem *);
bool modemCheckAttached(struct modem *, int timeout);
float modemReadFloat(struct modem *, int timeout);

bool modemHasPacket(struct modem *, size_t size);
bool modemGetPacket(struct modem *, void *mem, size_t size);

#endif
