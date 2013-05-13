
#include "modem.h"

#include "scheduler.h"

const char *IDENTIFY = "+++";
const char *CONNSTR = "CONNECT";
const int CONNSTRLEN = 7;

const char *DISCONNSTR = "NO CARRIER";
const int DISCONNSTRLEN = 10;

const char *GOODRESPONSE = "OK\r\n";
const int GOODRESPONSELEN = 2;

const char *CMD_RESET = "ATZ\n";

/* The ICL323 chip has a minimum limit on the maximum baud rate at 250000,
 * however we need a baud rate that the Due can run at, so choose the next
 * one down.
 */
const int MODEMBAUD = 115200;

float fltHalfToSingle(void *value);
float fltSingleToHalf(void *value);

#define PACKETSIZE 4

enum modemstate {
  UNATTACHED,
  ATTACHED,
  CONNECTED,
};

struct modem {
  /* Arduino object used for TTL serial I/O */
  USARTClass *serial;
  /* The state of the modem, fast method of keeping
   * track of whether the modem is attached, connected, or neither
   */
  enum modemstate state;
  /* Used to keep track of the state of the input.
   * Corresponds to an index in one of the strings above.
   */
  int statecheck;
  /* The following corresponds with SCU's specifications */
  /* A buffer for kayak commands, sent as 2 half precision values.
   * Used to temporarily store the next packets values
   */
  char buffer[4];
  /* The last complete packet recieved.
   * Only modified when a full command has been sent
   */
  char prevpacket[4];
  /* Where the packet in transit is relative to the number of bytes needed
   * for a full packet to be recieved.
   */
  int packetboundary;
  /* Whether or not a packet has been recieved since connecting to the
   * other modem
   */
  bool hasPacket;
  /* Whether or not SCU's base station expects a packet currently */
  bool needsPacket;
};

/* Terrible thing, used to remove any unneeded data from the serial object.
 * Preconditions: A valid serial object
 * Postconditions: A valid serial object with no buffered data
 */
void modemClear(USARTClass *serial);

struct modem *modemInit(USARTClass *serial, int timeout)
{
  /* Just verify that we have valid information */
  if(!serial || timeout <= 0)
    return NULL;
  /* Attempt to allocate memory for the modem */
  struct modem *modem = (struct modem *)malloc(sizeof(struct modem));
  if(!modem) {
    DEBUGSERIAL.print("Not enough memory!\r\n");
    return NULL;
  }
  /* Initialize our information to a state of ignorance */
  memset(modem, 0, sizeof(*modem));
  modem->state = UNATTACHED;
  modem->statecheck = 0;
  modem->packetboundary = 0;
  modem->serial = serial;
  modem->serial->begin(MODEMBAUD);
  /* Start fixing that ignorance */
  if(modemCheckAttached(modem, timeout)) {
    modem->state = ATTACHED;
  }
  else {
    /* We couldn't find a modem on the specified port,
     * so we're done here.
     */
    free(modem);
    return NULL;
  }
  registerTimer(100, (void (*)(void *))modemUpdate, modem);
  return modem;
}

void modemFree(struct modem *modem)
{
  /* Break out of any existing connections before trying to reset */
  modem->serial->write(IDENTIFY);
  delay(500);
  /* Put the modem in its default state */
  modem->serial->write(CMD_RESET);
  free(modem);
}

bool modemIsConn(struct modem *modem)
{
  if(modem->state == CONNECTED)
    return true;
  else
    return false;
}

bool modemCheckAttached(struct modem *modem, int timeout)
{
  /* The modem needs to be sent a +++ before it can accept commands.
   * After recieving it, it will respond with OK signifying that it is ready.
   * But don't look for the +++ for longer than timeout milliseconds
   */
  int inputstate = 0;
  for(; timeout > 0 && inputstate < GOODRESPONSELEN;) {
    modem->serial->write(IDENTIFY);
    /* The modem can take some time before it will respond, 
     * and won't respond if we interrupt it, so wait a couple seconds
     */
    delay(1000);
    timeout -= 1000;
    DEBUGPRINT("Checking for modem connection\r\n");
    if(modem->serial->available() > 0) {
      /* We got some information, now compare it to the expected return.
       * Basically an implementation of a miniature state machine for strings
       */
      while(modem->serial->available() > 0 && inputstate < GOODRESPONSELEN) {
	char check = modem->serial->read();
	if(check == GOODRESPONSE[inputstate]) {
	  /* Got the expected character, so advance to the next state */
	  inputstate++;
	}
	else {
	  if(check == GOODRESPONSE[0]) {
	    /* Not the expected character, but it does match the first expected
	     * character, so don't fall to the first state, but the second
	     */
	    inputstate = 1;
	  }
	  else {
	    /* No idea what was just recieved, so go back home */
	    inputstate = 0;
	  }
	}
      }
    }
  }
  /* Check that we reached the final state */
  if(inputstate == GOODRESPONSELEN) {
    /* We did :) */
    modem->state = ATTACHED;
    return true;
  }
  else {
    /* Nope :( */
    modem->state = UNATTACHED;
    return false;
  }
  /* For future reference:
   * > ats360?
   * < 00000040f000
   * is an unconnected modem, whereas
   * > ats360?
   * < 00000040f300
   * is connected. Similarly,
   * > ats361?
   * < 36
   * is unconnected, and
   * > ats361?
   * < 32
   * is connected. In the future, it would be better to query the
   * modems initial state, rather than just trying to clear it to
   * a known state.
   * Warning: These are not documented, and are only what
   * have been observed. Results may vary
   */
  /* Clear out anything else the modem may have sent */
  modemClear(modem->serial);
  /* Close any connections, so the modems state matches our default */
  modem->serial->write("ath\r\n");
  while(modem->serial->available() == 0);
  delay(10);
  modemClear(modem->serial);
}

void modemUpdate(struct modem *modem)
{
  /* Updates the modems state based on what has been recieved */
  if(modem->serial->available() > 0) {
    if(modem->state == CONNECTED)
      DEBUGPRINT("Checking for floating point values or for NO CARRIER\r\n");
    while(modem->serial->available() > 0) {
      if(modem->state != CONNECTED) {
	/* If the modem is not connected, then we need to look for
	 * when a connection is made. Nothing else should be returned
	 * by the modem, I hope.
	 * Do this with a state machine
	 */
	char check = modem->serial->read();
	if(CONNSTR[modem->statecheck] == check) {
	  modem->statecheck++;
	  if(modem->statecheck >= CONNSTRLEN) {
	    /* We have connected!!!1! */
	    modem->state = CONNECTED;
	    modem->statecheck = 0;
	    DEBUGSERIAL.print("Modem Connected\r\n");
	    modem->hasPacket = false;
	    modem->needsPacket = false;
	    modemClear(modem->serial);
	  }
	}
	else if(CONNSTR[0] == check) {
	  modem->statecheck = 1;
	}
	else {
	  modem->statecheck = 0;
	}
      }
      else {
	/* We must already be connected. Check for NO CARRIER */
	char check = modem->serial->read();
	modem->buffer[modem->packetboundary] = check;
	DEBUGSERIAL.print("Received data: ");
	DEBUGSERIAL.println(check, HEX);
	if(DISCONNSTR[modem->statecheck] == check) {
	  modem->statecheck++;
	  if(modem->statecheck >= DISCONNSTRLEN) {
	    modem->state = ATTACHED;
	    modem->statecheck = 0;
	    DEBUGSERIAL.print("Connection lost\r\n");
	    memset(modem->prevpacket, 0, sizeof(modem->prevpacket));
	  }
	}
	modem->packetboundary = (modem->packetboundary + 1) % 4;
	if(modem->packetboundary == 0) {
	  modem->needsPacket = true;
	  modem->hasPacket = true;
	  memcpy(modem->prevpacket, modem->buffer, sizeof(modem->prevpacket));
	}
      }
    }
    if(modem->state == CONNECTED) {
      DEBUGPRINT("Data read: ");
      DEBUGPRINT(modem->packetboundary);
      DEBUGPRINT("\r\nHas Packet: ");
      DEBUGPRINT(modemHasPacket(modem));
      short actual = (modem->prevpacket[0] << 8) + modem->prevpacket[1];
      float fwd = modemForwardPwr(modem);
      DEBUGPRINT("\r\nForward Power: ");
      DEBUGSERIAL.print(actual, HEX);
      DEBUGPRINT(" ");
      DEBUGPRINT(fwd);
      actual = (modem->prevpacket[2] << 8) + modem->prevpacket[3];
      DEBUGPRINT("\r\n");
      float rot = modemRotationPwr(modem);
      DEBUGPRINT("\r\nRotation Power: ");
      DEBUGSERIAL.print(actual, HEX);
      DEBUGPRINT(" ");
      DEBUGPRINT(rot);
      DEBUGPRINT("\r\n");
    }
  }
  registerTimer(100, (void (*)(void *))modemUpdate, modem);
}

void modemSendPacket(struct modem *modem, void *packet, size_t size)
{
  if(modem->state != CONNECTED)
    return;
  byte *b = (byte *)packet;
  for(int i = 0; i < size; i++) {
    modem->serial->write(b[i]);
  }
}

float modemForwardPwr(struct modem *modem)
{
  if(modem->state != CONNECTED)
    return 0;
  short value = (modem->prevpacket[0] << 8) + modem->prevpacket[1];
  if(value > 127)
    value = -value + 127;
  float pwr = value / 127.0;
  DEBUGPRINT("\r\nForward Power\r\n");
  return fltHalfToSingle(modem->prevpacket);
}

float modemRotationPwr(struct modem *modem)
{
  if(modem->state != CONNECTED)
    return 0;
  short value = (modem->prevpacket[2] << 8) + modem->prevpacket[3];
  if(value > 127)
    value = -value + 127;
  float pwr = value / 127.0;
  DEBUGPRINT("\r\nRotation Power\r\n");
  return fltHalfToSingle(&(modem->prevpacket[2]));
}

bool modemHasPacket(struct modem *modem)
{
  return modem->hasPacket;
}

void modemClear(USARTClass *serial)
{
  /* Clear the buffer. I'd really rather just hack the Arduino library
   * so it doesn't waste CPU cycles */
  while(serial->available())
    serial->read();
}

bool modemNeedsPacket(struct modem *modem)
{
  return modem->needsPacket;
}

short fltSingleToHalf(float value)
{
  if(value == 0.00)
    return 0;
  int negative = ((((*((int *) &value)) & 0x80000000)) >> 16);
  int exponent = ((((*((int *) &value)) & 0x7f800000) >> 23) - 127 + 15) << 10;
  int mantissa = ((((*((int *) &value)) & 0x007fe000)) >> 13);
  DEBUGPRINT("Hex form: 0x");
  DEBUGPRINTHEX(*((unsigned *)&value));
  DEBUGPRINT("\r\nConverting down\r\nNegative: ");
  DEBUGPRINTHEX(negative);
  DEBUGPRINT("\r\nExponent: ");
  DEBUGPRINTHEX(exponent);
  DEBUGPRINT("\r\nMantissa: ");
  DEBUGPRINTHEX(mantissa);
  DEBUGPRINT("\r\n");
  negative &= 0x8000;
  exponent &= 0x7c00;
  mantissa &= 0x03ff;
  short final = negative + exponent + mantissa;
  return final;
}

float fltHalfToSingle(void *value)
{
  union dest {
    float fp;
    unsigned int val;
  } data;
  union src {
    unsigned short value;
    unsigned char bytes[2];
  } *source = (union src *)value;
  if(source->value == 0)
    return 0.0;
  int sign = (source->value & 0x8000);
  DEBUGPRINT("\r\nInitial sign: ");
  DEBUGPRINTHEX(sign);
  DEBUGPRINT("\r\n");
  sign <<= 16;
  int exp = (((source->value & 0x7C00) >> 10) - 15 + 127);
  exp <<= 23;
  int mantissa = (source->value & 0x3ff);
  mantissa <<= 13;
  DEBUGPRINT("Converting up:\r\nValue: ");
  DEBUGPRINTHEX(source->value);
  DEBUGPRINT("\r\nSign: ");
  DEBUGPRINT(sign);
  DEBUGPRINT("\r\nExponent: ");
  DEBUGPRINT(exp);
  DEBUGPRINT("\r\nMantissa: ");
  DEBUGPRINT(mantissa);
  DEBUGPRINT("\r\nResult: ");
  data.val = sign + exp + mantissa;
  DEBUGPRINT(data.fp);
  DEBUGPRINT("\r\n");
  return data.fp;
}
