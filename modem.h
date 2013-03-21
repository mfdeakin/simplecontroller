
#ifndef _MODEM_H_
#define _MODEM_H_

#include <Arduino.h>
#include "include.h"

struct modem;

/* Initializes the modem and a datastructure to keep track of it
 * Tries to connect for timeout seconds
 */
struct modem *modemInit(USARTClass *serial, int timeout);
/* Frees the modem, disconnects the modem */
void modemFree(struct modem *);

void modemUpdate(struct modem *);

bool modemIsConn(struct modem *);
bool modemCheckAttached(struct modem *, int timeout);
float modemReadFloat(struct modem *, int timeout);

float modemForwardPwr(struct modem *modem);
float modemRotationPwr(struct modem *modem);

bool modemHasPacket(struct modem *);
bool modemGetPacket(struct modem *, void *mem, size_t size);

#endif
