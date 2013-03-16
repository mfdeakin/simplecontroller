
#include "scheduler.h"

#include <Arduino.h>
#include <assert.h>
#include "heap.h"
#include "list.h"

typedef struct event {
	/* Ticks are measured in ms I believe */
	unsigned rticks;
	void (*proc)(void *data);
	void *data;
} event;

heap *queued;

int cmpTime(event *lhs, event *rhs);
void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);

void registerTimer(unsigned deltams, void (*proc)(void *data), void *data)
{
	event *evt = (event *)malloc(sizeof(event));
	evt->rticks = deltams + GetTickCount();
	evt->proc = proc;
	evt->data = data;
	hpAdd(queued, evt);
	event *newtop = (event *)hpPeek(queued);
	float freq = 1000.0f / newtop->rticks;
	startTimer(TC1, 0, TC3_IRQn, freq);
}

// void TC3_Handler()
// {
// 	noInterrupts();
// 	event *evt = (event *)hpTop(queued);
// 	event *next = (event *)hpPeek(queued);
// 	interrupts();
// 	evt->proc(evt->data);
// 	free(evt);
// 	float freq = 1000.0f / next->rticks;
// 	TC_GetStatus(TC1, 0);
// 	startTimer(TC1, 0, TC3_IRQn, freq);
// }

void schedulerInit(void)
{
	queued = hpCreate((int (*)(void *, void *))cmpTime);
}

int cmpTime(event *lhs, event *rhs)
{
	assert(lhs && rhs);
	return rhs->rticks - lhs->rticks;
}

void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency)
{
        pmc_set_writeprotect(false);
        pmc_enable_periph_clk((uint32_t)irq);
        TC_Configure(tc, channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4);
        uint32_t rc = VARIANT_MCK/128/frequency; //128 because we selected TIMER_CLOCK4 above
        TC_SetRA(tc, channel, rc/2); //50% high, 50% low
        TC_SetRC(tc, channel, rc);
        TC_Start(tc, channel);
        tc->TC_CHANNEL[channel].TC_IER=TC_IER_CPCS;
        tc->TC_CHANNEL[channel].TC_IDR=~TC_IER_CPCS;
        NVIC_EnableIRQ(irq);
}
