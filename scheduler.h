
#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

struct scheduler;

struct scheduler *schedulerInit(void);
void registerTimer(unsigned deltams, void (*proc)(void *data), void *data);
bool schedulerProcessEvents(struct scheduler *s);

#endif
