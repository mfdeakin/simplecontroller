
#include "scheduler.h"
#include "include.h"

#include <Arduino.h>
#include <assert.h>
#include "semaphore.h"
#include "heap.h"
#include "list.h"

typedef struct event {
  /* Ticks are in approximage ms, according to the libsam library.
   * timetick.h, line 38
   * rticks is the absolute number of ticks before the 
   */
  unsigned rticks;
  void (*proc)(void *data);
  void *data;
} event;

struct scheduler {
  heap *queued;
  heap *ready;
  int readysem;
} *scheduler = NULL;

int cmpTime(event *lhs, event *rhs);
void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);

void registerTimer(unsigned deltams, void (*proc)(void *data), void *data)
{
  event *evt = (event *)malloc(sizeof(event));
  if(!evt) {
    DEBUGSERIAL.print("Could not allocate memory to register timer!!!\r\n");
  }
  evt->rticks = deltams + GetTickCount();
  evt->proc = proc;
  evt->data = data;
  semDown(&scheduler->readysem);
  hpAdd(scheduler->queued, evt);
  event *newtop = (event *)hpPeek(scheduler->queued);
  semUp(&scheduler->readysem);
  float freq = 1000.0f / (newtop->rticks - GetTickCount());
  DEBUGPRINT("Registering timer for ");
  DEBUGPRINT(deltams);
  DEBUGPRINT(" ms from now with frequency ");
  DEBUGPRINT(freq);
  DEBUGPRINT(".\r\n");
  startTimer(TC1, 0, TC3_IRQn, freq);
}

void TC3_Handler()
{
  /* This is called by the sam3x code when it recieves a timer interrupt.
   * It will not be called again before the GetStatus is called.
   * We need to do three things:
   * Get the event to be processed
   * Put it into the ready queue
   * Start the timer for the next event
   */
  int t1 = GetTickCount();
  int islocked = semTryDown(&scheduler->readysem);
  if(islocked) {
    event *evt = (event *)hpTop(scheduler->queued);
    event *next = (event *)hpPeek(scheduler->queued);
    
    hpAdd(scheduler->ready, evt);
    /* Let any other timers on TC1 go */
    TC_GetStatus(TC1, 0);
    semUp(&scheduler->readysem);
    /* We register the next timer (or disable the timer)
     * before we execute the event code because the event
     * code may need to register another timer
     */
    DEBUGPRINT("\r\nHandling timer event\r\n");
    if(next) {
      int deltat = next->rticks - t1;
      /* deltat may be negative */
      if(deltat < 1) {
	DEBUGPRINT("Starting timer immediately from now\r\n");
	/* We have another event which is due now, so process it ASAP.
	 * I'm not certain how fast we can get the timer to go,
	 * hopefully 0.1 ms is good enough.
	 * Faster timers don't seem to work
	 */
	startTimer(TC1, 0, TC3_IRQn, 10000);
      }
      else {
	float freq = 1000.0f / deltat;
	DEBUGPRINT("Starting timer for ");
	DEBUGPRINT(deltat);
	DEBUGPRINT(" ms from now.\r\n");
	startTimer(TC1, 0, TC3_IRQn, freq);
      }
    }
    else {
      /* Turn the clock off, we shouldn't need it
       * Save our entropy!!!
       */
      DEBUGPRINT("Turning timer off\r\n");
      pmc_disable_periph_clk((uint32_t)TC3_IRQn);
    }
  }
  else {
    /* We didn't get the lock, so try again after letting the other code go */
    startTimer(TC1, 0, TC3_IRQn, 10000);
  }
}

struct scheduler *schedulerInit(void)
{
  /* The scheduler is a singleton :( */
  if(scheduler)
    return NULL;
  scheduler = (struct scheduler *)malloc(sizeof(struct scheduler));
  if(!scheduler) {
    DEBUGSERIAL.print("Could not allocate enough memory!\r\n");
    return NULL;
  }
  scheduler->queued = hpCreate((int (*)(void *, void *))cmpTime);
  scheduler->ready = hpCreate((int (*)(void *, void *))cmpTime);
  scheduler->readysem = 1;
  return scheduler;
}

bool schedulerProcessEvents(struct scheduler *s)
{
  event *evt = (event *)hpTop(s->ready);
  if(evt) {
    assert(evt->proc);
    evt->proc(evt->data);
    return true;
  }
  return false;
}

int cmpTime(event *lhs, event *rhs)
{
  assert(lhs && rhs);
  return rhs->rticks - lhs->rticks;
}

void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency)
{
  /* Black magic box */
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk((uint32_t)irq);
  TC_Configure(tc, channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4);
  /* 128 because we selected TIMER_CLOCK4 above */
  uint32_t rc = VARIANT_MCK / 128 / frequency;
  TC_SetRA(tc, channel, rc / 2); //50% high, 50% low
  TC_SetRC(tc, channel, rc);
  TC_Start(tc, channel);
  tc->TC_CHANNEL[channel].TC_IER = TC_IER_CPCS;
  tc->TC_CHANNEL[channel].TC_IDR = ~TC_IER_CPCS;
  NVIC_EnableIRQ(irq);
}
