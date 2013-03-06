
#include <Arduino.h>
#include "list.h"

void initModem(void);
void initMotorController(void);

void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);

#define DEBUGSERIAL Serial
#define MOTORSERIAL Serial1
#define MODEMSERIAL Serial2

void motorWriteByte(byte b);
void motorWriteString(char *str);
char motorReadByte(void);

bool connected = false;
int leftmotor = 0, rightmotor = 0;

char *readLine(HardwareSerial *sio);

void setup(void)
{
  DEBUGSERIAL.begin(9600);
	listFree(listCreate());
  initModem();
  initMotorController();
  /* Start a timer to control the motor every 100 milliseconds (10 Hz)*/
  startTimer(TC1, 0, TC3_IRQn, 10);
}

void TC3_Handler()
{
  /* Accept the interrupt, so it will be called again */
  TC_GetStatus(TC1, 0);
  const int BUFSIZE = 100;
  char buffer[BUFSIZE];
  sprintf(buffer, "!A%d\r", rightmotor);
  motorWriteString(buffer);
  sprintf(buffer, "!B%d\r", leftmotor);
  motorWriteString(buffer);
}

void loop(void)
{
  const int BUFSIZE = 100;
  char buffer[BUFSIZE];
  char nil = 0; //Protective byte

  if(MODEMSERIAL.available() > 0) {
    if(!connected) {
      connected = true;
      for(int i = 0; i < MODEMSERIAL.available(); i++) {
      /* I think this works... */
        if(MODEMSERIAL.read() != "CONNECTED"[i]) {
          connected = false;
          break;
        }
      }
      if(connected) {
        DEBUGSERIAL.println("Modems connected");
      }
    }
    /* Read until we fill up the buffer or we hit a carriage return */
    buffer[0] = MODEMSERIAL.read();
    int i;
    for(i = 1;
        i < BUFSIZE && buffer[i - 1] != '\r';
        i++) {
      buffer[i] = MODEMSERIAL.read();
    }
    buffer[i] = 0;
    /* Determine if we are to change the power sent to a motor */
    int number;
    sscanf(buffer, "%d", &number);
    if(buffer[i - 1] == 'r')
      rightmotor = number & 0x7f;
    else if(buffer[i - 1] == 'l')
      leftmotor = number & 0x7f;
  }
}

void initModem(void)
{
  DEBUGSERIAL.write("Initializing Modem\n");
  const int modembaud = 19200;
  MODEMSERIAL.begin(modembaud);
  const char *states = "OK";
  /* The modem needs to be sent a +++ before it can accept commands.
   * After recieving it, it will respond with OK signifying that it is ready.
   */
  for(int inputstate; inputstate < 2;) {
    MODEMSERIAL.write("+++");
    /* The modem can take some time before it will respond, so wait a second */
    delay(5000);
    if(MODEMSERIAL.available() > 0) {
      char *input = (char *)malloc(sizeof(char[MODEMSERIAL.available() + 1]));
      input[MODEMSERIAL.available()] = 0;
      DEBUGSERIAL.print(MODEMSERIAL.available());
      DEBUGSERIAL.print(" bytes recieved\n");
      for(int i = 0; MODEMSERIAL.available() > 0; i++) {
        input[i] = (char)MODEMSERIAL.read();
        DEBUGSERIAL.print(input[i], HEX);
        DEBUGSERIAL.print(' ');
        if(input[i] == states[inputstate]) {
          inputstate++;
        }
      }
      DEBUGSERIAL.write('\n');
      DEBUGSERIAL.write(input);
      DEBUGSERIAL.write('\n');
      DEBUGSERIAL.print("inputstate: ");
      DEBUGSERIAL.println(inputstate);
      free(input);
    }
  }
  DEBUGSERIAL.println("Modem initialized");
}

void initMotorController(void)
{
  DEBUGSERIAL.println("Initializing motor controller");
  const int motorbaud = 9600;
  MOTORSERIAL.begin(motorbaud);
  /* The motor controller expects 10 carriage returns before it will accept
   * commands from the serial port. It will then start talking to us.
   */
  const char *states = "OK";
  for(int inputstate = 0; inputstate < 2;) {
    motorWriteString("\r\r\r\r\r\r\r\r\r\r");
    /* The motor controller can take some time before it will respond,
     * so wait a second */
    delay(100);
    if(MOTORSERIAL.available() > 0) {
      char *input = (char *)malloc(sizeof(char[MOTORSERIAL.available() + 1]));
      input[MOTORSERIAL.available()] = 0;
      DEBUGSERIAL.print(MOTORSERIAL.available());
      DEBUGSERIAL.print(" bytes recieved\n");
      for(int i = 0; MOTORSERIAL.available() > 0; i++) {
        input[i] = motorReadByte();
        DEBUGSERIAL.print(input[i], HEX);
        DEBUGSERIAL.print(' ');
        if(input[i] == states[inputstate]) {
          inputstate++;
        }
      }
      DEBUGSERIAL.write('\n');
      DEBUGSERIAL.write(input);
      DEBUGSERIAL.write('\n');
      free(input);
    }
  }
  DEBUGSERIAL.println("Motor controller initialized");
}

char motorReadByte(void)
{
  return MOTORSERIAL.read() & 0x7f;
}

void motorWriteString(char *str)
{
  for(int i = 0; str[i]; i++) {
    motorWriteByte(str[i]);
  }
}

void motorWriteByte(byte b)
{
  b <<= 1;
  byte parity = 0;
  byte temp = b;
  while(temp) {
    parity ^= temp & 0x80;
    temp <<= 1;
  }
  b = (b >> 1) | parity;
  MOTORSERIAL.write(b);
}

/* Black Magic */
void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency) {
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

