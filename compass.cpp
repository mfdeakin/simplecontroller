
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
  struct compass *compass =
    (struct compass *)malloc(sizeof(struct compass));
  if(!compass) {
    DEBUGSERIAL.print("Could not allocate memory for the compass!\r\n");
    return NULL;
  }
  compass->wire = wire;
  compass->wire->begin();
  compass->bearing = 0.0 / 0.0;
  registerTimer(100, (void (*)(void *))compassUpdate, compass);
  return compass;
}

void compassUpdate(struct compass *compass)
{
  compass->wire->beginTransmission(COMPASSADDRESS);
  compass->wire->write(2);
  compass->wire->endTransmission();
  
  compass->wire->requestFrom(COMPASSADDRESS, 2);

//Return error float if no compass present
  union {
    byte byteval[2];
    short intval;
  } bytes;
  while(!compass->wire->available());
  bytes.byteval[1] = compass->wire->read();
  bytes.byteval[0] = compass->wire->read();
//Read 2 bytes, combine and divide by 10 to return value to one
//decimal value
  compass->bearing = bytes.intval / 10.0;
  registerTimer(100, (void (*)(void *))compassUpdate, compass);
}

float compassBearing(struct compass *compass)
{ 
  return compass->bearing;
}
