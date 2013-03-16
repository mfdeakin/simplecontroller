
#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

struct scheduler;

void schedulerInit(void);
void registerTimer(unsigned deltams, void (*proc)(void *data), void *data);

#endif
