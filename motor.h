
#ifndef _MOTOR_H_
#define _MOTOR_H_

#define RELEASE_VERSION

#include <Arduino.h>
#include "include.h"

struct motorctrl;

struct channelpair {
	int cA, cB;
};

struct motorctrl *motorInit(USARTClass *serial, int timeout);
/* Frees the motor controller, turns off the motors */
void motorFree(struct modem *);

bool motorWriteCmd(struct motorctrl *, const char *cmd,
									 void *buffer, size_t size, int timeout);

void motorSetSpeed(struct motorctrl *, float forward, float rotate);
struct channelpair motorCheckAmp(struct motorctrl *);
struct channelpair motorCheckVolt(struct motorctrl *);

#endif
