
#include "scheduler.h"
#include "include.h"

#include <Arduino.h>
#include <assert.h>
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

heap *queued;

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
  hpAdd(queued, evt);
  event *newtop = (event *)hpPeek(queued);
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
  int t1 = GetTickCount();
  event *evt = (event *)hpTop(queued);
  event *next = (event *)hpPeek(queued);
  /* Let any other timers on TC1 go */
  TC_GetStatus(TC1, 0);
  /* We register the next timer (or disable the timer)
   * before we execute the event code because the event
   * code may need to register another timer
   */
  DEBUGPRINT("\r\nHandling timer event\r\n");
  if(next) {
    int deltat = next->rticks - t1;
    if(deltat < 1) {
      DEBUGPRINT("Starting timer immediately from now\r\n");
      /* We have another event which is due now, so process it ASAP */
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
  if(evt) {
    DEBUGPRINT("Running event code\r\n");
    if(evt->proc)
      evt->proc(evt->data);
    free(evt);
  }
}

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
