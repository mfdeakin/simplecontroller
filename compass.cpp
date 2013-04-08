
#include "include.h"
#include "compass.h"

#include "scheduler.h"

#define COMPASSADDRESS 0x60

struct compass {
  TwoWire *wire;
  float bearing;
};

void compassUpdate(struct compass *compass);

struct compass *compassInit(TwoWire *wire)
{
  struct compass *cmp =
    (struct compass *)malloc(sizeof(struct compass));
  if(!cmp) {
    DEBUGSERIAL.print("Could not allocate memory for the compass!\r\n");
    return NULL;
  }
  cmp->wire = wire;
  cmp->wire->begin();
  cmp->bearing = 0.0 / 0.0;
  registerTimer(100, (void (*)(void *))compassUpdate, cmp);
  return cmp;
}

void compassUpdate(struct compass *cmp)
{
  DEBUGPRINT("Compass Update\r\n");
  cmp->wire->beginTransmission(COMPASSADDRESS);
  cmp->wire->write(2);
  cmp->wire->endTransmission();
  
  cmp->wire->requestFrom(COMPASSADDRESS, 2);

//Return error float if no compass present
  union {
    byte byteval[2];
    short intval;
  } bytes;
  while(!cmp->wire->available());
  bytes.byteval[1] = cmp->wire->read();
  bytes.byteval[0] = cmp->wire->read();
//Read 2 bytes, combine and divide by 10 to return value to one
//decimal value
  cmp->bearing = bytes.intval / 10.0;
  registerTimer(100, (void (*)(void *))compassUpdate, cmp);
}

float compassBearing(struct compass *cmp)
{ 
  return cmp->bearing;
}
