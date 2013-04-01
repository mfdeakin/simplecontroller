
#include <Arduino.h>
#include <Wire/Wire.h>
#include "include.h"
#include "list.h"
#include "modem.h"
#include "motor.h"
#include "compass.h"

#include "TinyGPS.h"

void initMotorController(void);

struct kayak {
  TinyGPS gpsdata;
  struct modem *modem;
  struct motorctrl *motor;
  unsigned powerused;
  float heading;
} kayak;

void sendPacket();
void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);
void Wire_Init(void);

void setup(void)
{
  DEBUGSERIAL.begin(9600);
  GPSSERIAL.begin(38400);
  Wire.begin();

  DEBUGPRINT("Attempting to connect the modem\r\n");
  kayak.modem = modemInit(&MODEMSERIAL, 1000);
  if(!kayak.modem) {
    DEBUGSERIAL.print("Modem not connected\r\n");
  }
  else {
    DEBUGSERIAL.print("Modem connected\r\n");
  }
  DEBUGPRINT("Attempting to connect the motor controller\r\n");
  kayak.motor = motorInit(&MOTORSERIAL, 100);
  if(!kayak.motor) {
    DEBUGSERIAL.print("Motor controller not connected\r\n");
  }
  else {
    DEBUGSERIAL.print("Motor controller connected\r\n");
  }
  startTimer(TC1, 0, TC3_IRQn, 1);
}

void loop()
{
  delay(100);
  if(kayak.motor) {
    motorCheckVolt(kayak.motor);
    motorCheckAmp(kayak.motor);
    modemUpdate(kayak.modem);
  }
  if(kayak.modem && kayak.motor && modemHasPacket(kayak.modem)) {
    motorSetSpeed(kayak.motor,
		  modemForwardPwr(kayak.modem),
		  modemRotationPwr(kayak.modem));
  }
  if(GPSSERIAL.available() > 0) {
    bool encoded = false;
    // DEBUGSERIAL.write("Reading GPS Data:\r\n");
    while(GPSSERIAL.available() > 0 && !encoded){
      byte value = GPSSERIAL.read();
      // DEBUGSERIAL.write(value);
      encoded = kayak.gpsdata.encode(value);
    }
    DEBUGSERIAL.write("\r\n");
  }
  kayak.heading = compassBearing();
  if(modemNeedsPacket(kayak.modem)) {
    sendPacket();
  }
}

void sendPacket()
{
  struct {
    /* Compass Data */
    short heading;
    /* GPS Data */
    float time;
    int lat;
    char lathem;
    int lng;
    char lnghem;
    byte satellites;
    short hdilution;
    short course;
    short magcourse;
    short groundspeed;
  } packet;
  memset(&packet, 0, sizeof(packet));
  packet.heading = kayak.heading;
  int year;
  byte month, day, hour, minute, second;
  kayak.gpsdata.crack_datetime(&year, &month, &day, &hour,
			       &minute, &second);
  packet.time = hour * 3600.0f + minute * 60.0f + second;
  long lat, lng;
  kayak.gpsdata.get_position(&lat, &lng);
  if(lat < 0) {
    lat = -lat;
    packet.lathem = 'S';
  }
  else {
    packet.lathem = 'N';
  }
  if(lng < 0) {
    lng = -lng;
    packet.lnghem = 'E';
  }
  else {
    packet.lnghem = 'W';
  }
  packet.lat = (int)lat;
  packet.lng = (int)lng;
  packet.satellites = (byte)kayak.gpsdata.satellites();
  packet.hdilution = (short)kayak.gpsdata.hdop();
  packet.course = (short)kayak.gpsdata.f_course() * 100;
  packet.magcourse = (short)kayak.gpsdata.f_course() * 100;
  packet.groundspeed = (short)kayak.gpsdata.f_speed_kmph();
  DEBUGSERIAL.print("\r\nLatitude: ");
  DEBUGSERIAL.print(packet.lat);
  DEBUGSERIAL.println(packet.lathem);
  DEBUGSERIAL.print("Longitude: ");
  DEBUGSERIAL.print(packet.lng);
  DEBUGSERIAL.println(packet.lnghem);
  DEBUGSERIAL.print("GPS Heading: ");
  DEBUGSERIAL.println(packet.course);
  DEBUGSERIAL.print("Compass Heading: ");
  DEBUGSERIAL.println(packet.heading);
  DEBUGSERIAL.print("Speed (km/h): ");
  DEBUGSERIAL.println(packet.groundspeed);
  DEBUGSERIAL.print("Number of satellites: ");
  DEBUGSERIAL.println(packet.satellites);
  DEBUGSERIAL.print("Horizontal Dilution: ");
  DEBUGSERIAL.println(packet.hdilution);
  if(kayak.modem)
    modemSendPacket(kayak.modem, &packet, sizeof(packet));
}
