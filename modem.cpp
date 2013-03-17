
#include "modem.h"

const char *IDENTIFY = "+++";
const char *CONNSTR = "CONNECT";
const int CONNSTRLEN = 7;

const char *DISCONNSTR = "NO CARRIER";
const int DISCONNSTRLEN = 10;

const char *GOODRESPONSE = "OK\r\n";
const int GOODRESPONSELEN = 2;

const char *CMD_RESET = "ATZ\n";

#define PACKETSIZE 4

enum modemstate {
  UNATTACHED,
  ATTACHED,
  CONNECTED,
};

struct modem {
  USARTClass *serial;
  enum modemstate state;
  float lngVel;
  float rotVel;
  int statecheck;
  /* Buffer is used for recieving packets in SCU's format */
  char buffer[4];
  char prevpacket[4];
  int packetboundary;
  bool hasPacket;
};

void modemClear(USARTClass *serial);

struct modem *modemInit(USARTClass *serial, int timeout)
{
  if(!serial)
    return NULL;
  struct modem *modem = (struct modem *)malloc(sizeof(struct modem));
  if(!modem) {
    DEBUGSERIAL.print("Not enough memory!\r\n");
    return NULL;
  }
  memset(modem, 0, sizeof(*modem));
  modem->state = UNATTACHED;
  modem->statecheck = 0;
  modem->packetboundary = 0;
  modem->serial = serial;
  const int modembaud = 19200;
  modem->serial->begin(modembaud);
  if(modemCheckAttached(modem, timeout)) {
    modem->state = ATTACHED;
  }
  else {
    free(modem);
    modem = NULL;
  }
  return modem;
}

void modemFree(struct modem *modem)
{
  /* Break out of any existing connections before trying to reset */
  modem->serial->write(IDENTIFY);
  delay(100);
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
      while(modem->serial->available() > 0 && inputstate < GOODRESPONSELEN) {
	char check = modem->serial->read();
	if(check == GOODRESPONSE[inputstate]) {
	  inputstate++;
	}
	else {
	  if(check == GOODRESPONSE[0]) {
	    inputstate = 1;
	  }
	  else {
	    inputstate = 0;
	  }
	}
      }
    }
  }
  if(inputstate == GOODRESPONSELEN) {
    modem->state = ATTACHED;
    return true;
  }
  else {
    modem->state = UNATTACHED;
    return false;
  }
  modemClear(modem->serial);
  modem->serial->write("+++");
  while(modem->serial->available() == 0);
  delay(10);
  modemClear(modem->serial);
  modem->serial->write("ath\r\n");
  modem->serial->flush();
  while(modem->serial->available() == 0);
  delay(10);
  modemClear(modem->serial);
}

void modemUpdate(struct modem *modem)
{
  if(modem->serial->available() > 0) {
    if(modem->state == CONNECTED)
      DEBUGPRINT("Checking for floating point values or for NO CARRIER\r\n");
    while(modem->serial->available() > 0) {
      if(modem->state != CONNECTED) {
	char check = modem->serial->read();
	if(CONNSTR[modem->statecheck] == check) {
	  modem->statecheck++;
	  if(modem->statecheck >= CONNSTRLEN) {
	    modem->state = CONNECTED;
	    modem->statecheck = 0;
	    DEBUGSERIAL.print("Modem Connected\r\n");
	    modem->hasPacket = false;
	    /* Discard all of the other junk */
	    while(modem->serial->available() > 0 && modem->serial->read() != '\n');
	    /* An extra byte is sent, remove it */
	    modem->serial->read();
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
	DEBUGPRINT(check);
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
      DEBUGPRINT("\r\nForward Power: ");
      DEBUGSERIAL.print(modem->prevpacket[0], HEX);
      DEBUGSERIAL.print(modem->prevpacket[1], HEX);
      DEBUGPRINT(" ");
      DEBUGPRINT(modemForwardPwr(modem));
      DEBUGPRINT("\r\nRotation Power: ");
      DEBUGSERIAL.print(modem->prevpacket[2], HEX);
      DEBUGSERIAL.print(modem->prevpacket[3], HEX);
      DEBUGPRINT(" ");
      DEBUGPRINT(modemRotationPwr(modem));
      DEBUGPRINT("\r\n");
    }
  }
}

float modemForwardPwr(struct modem *modem)
{
  float pwr;
  if(halfp2singles(&pwr, modem->prevpacket, 1)) {
    DEBUGSERIAL.print("Not in IEEE 16 bit format!\r\n");
  }
  return pwr;
}

float modemRotationPwr(struct modem *modem)
{
  float pwr;
  if(halfp2singles(&pwr, &modem->prevpacket[2], 1)) {
    DEBUGSERIAL.print("Not in IEEE 16 bit format!\r\n");
  }
  return pwr;
}

/* Reads a 16 bit floating point value
 * Returns an equivalent 32 bit floating point value
 */
float modemReadFloat(struct modem *modem, int timeout)
{
  modem->serial->setTimeout(timeout);
  char buffer[1024];
  bool cleanup = true;
  while(cleanup) {
    buffer[0] = modem->serial->read();
    if((buffer[0] >= '0' && buffer[0] <= '9') || buffer[0] == '.')
      cleanup = false;
  }
  for(int i = 1; modem->serial->available() && i < 1024; i++) {
    buffer[i] = modem->serial->read();
    buffer[i + 1] = 0;
  }
  float value;
  sscanf(buffer, "%f", &value);
  if(value > 1.0)
    value = 1.0;
  else if(value < -1.0)
    value = -1.0;
  return value;
  unsigned halffloat;
  modem->serial->readBytes((char *)&halffloat, 2);
  float fullfloat;
  halfp2singles(&fullfloat, &halffloat, 1);
  return fullfloat;
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
