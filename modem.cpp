
#include "modem.h"

const char *IDENTIFY = "+++";
const char *CONNSTR = "CONNECTED";
const char *DISCONNSTR = "CARRIER LOST";

const char *GOODRESPONSE = "OK\r\n";
const int GOODRESPONSELEN = 2;

const char *CMD_RESET = "ATZ\n";

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
};

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
}

/* Reads a 16 bit floating point value
 * Returns an equivalent 32 bit floating point value
 */
float modemReadFloat(struct modem *modem, int timeout)
{
  modem->serial->setTimeout(timeout);
  unsigned halffloat;
  modem->serial->readBytes((char *)&halffloat, 2);
  float fullfloat;
  halfp2singles(&fullfloat, &halffloat, 1);
  return fullfloat;
}

bool modemHasPacket(struct modem *modem, size_t size)
{
  if(modem->serial->available() >= size) {
    return true;
  }
  return false;
}

bool modemGetPacket(struct modem *modem, void *mem, size_t size)
{
  if(modemHasPacket(modem, size)) {
    
    return true;
  }
}
