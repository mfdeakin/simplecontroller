
#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <Arduino.h>
#include "include.h"

struct motorctrl;

struct channelpair {
	int cA, cB;
};

struct motorctrl *motorInit(USARTClass *serial, int timeout);
/* Frees the modem, disconnects the modem */
void motorFree(struct modem *);

bool motorWriteCmd(struct motorctrl *, const char *cmd,
									 void *buffer, size_t size, int timeout);

void motorSetSpeed(struct motorctrl *, float forward, float rotate);
struct channelpair motorCheckAmp(struct motorctrl *);
struct channelpair motorCheckVolt(struct motorctrl *);

#endif
