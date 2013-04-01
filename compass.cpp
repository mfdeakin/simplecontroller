
#include "include.h"
#include "compass.h"

#define COMPASSADDRESS 0x60

float compassBearing()
{ 
  Wire.beginTransmission(COMPASSADDRESS);
  Wire.write(2);
  Wire.endTransmission();
  
  Wire.requestFrom(COMPASSADDRESS, 2);

//Return error float if no compass present
  union {
    byte byteval[2];
    short intval;
  } bytes;
  while(!Wire.available());
  bytes.byteval[1] = Wire.read();
  bytes.byteval[0] = Wire.read();
//Read 2 bytes, combine and divide by 10 to return value to one
//decimal value
  float bearing = bytes.intval;
  bearing /= 10.0;
  DEBUGSERIAL.print("\r\nCompass Value: ");
  DEBUGSERIAL.println(bytes.intval);
  return bearing;
}
