
#include <Arduino.h>
#include <Wire/Wire.h>
#include "include.h"
#include "scheduler.h"
#include "list.h"
#include "modem.h"
#include "motor.h"
#include "compass.h"

#include "TinyGPS.h"

/* Group our stuff used for our program, not just program wide globals */
struct kayak {
  /* Used to parse GPS data */
  TinyGPS gpsdata;
  /* Compass object, keeps track of the bearing of the kayak */
  struct compass *compass;
  /* Modem object, keeps track of the state of the modem */
  struct modem *modem;
  /* Motor controller object, keeps track of the state of the motors and battery,
   * provides interface between the two
   */
  struct motorctrl *motor;
  /* Scheduler object, used to schedule jobs and what not */
  struct scheduler *scheduler;
  /* The total power used by the motors */
  unsigned powerused;
} kayak;

/* Used to send all of the data that Santa Clara's packet format specifies */
void sendPacket();

void enableTRNG(void);
uint32_t trandom(void);

void setup(void)
{
  /* Basic initialization for assumed pieces of hardware...
   * Debug output (USB connection), GPS (3.3 V TTL Serial)
   * and the compass (I2C)
   */
  DEBUGSERIAL.begin(115200);
  kayak.scheduler = schedulerInit();
  GPSSERIAL.begin(38400);
  
  /* The more involved pieces of hardware are handled in a more
   * more robust manner... Modem and motor controller
   */
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
  DEBUGPRINT("Initializing the compass\r\n");
  kayak.compass = compassInit(&COMPASSWIRE);
}

void loop()
{
  /* Puts the processor to sleep until an interrupt occurs (such as a timer,
   * or serial input.
   * From:
   * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/CIHCAEJD.html
   */
  __WFI();
  /* The scheduler may have gotten some events to process, so let it run */
  while(schedulerProcessEvents(kayak.scheduler));
  /* For sanity, don't pound the motor controller with commands
   * These should be in the scheduler when it's ready
   */
  delay(100);
  
  if(kayak.motor) {
    /* Update our statistics on the motor controllers power usage */
    motorCheckWatt(kayak.motor);
    motorCheckAmp(kayak.motor);
  }
  /* Update the powers sent to the motor controller */
  if(kayak.modem && kayak.motor && modemHasPacket(kayak.modem)) {
    motorSetSpeed(kayak.motor,
		  modemForwardPwr(kayak.modem),
		  modemRotationPwr(kayak.modem));
  }
  /* Update the GPS information; longitude, latitude, number of satellites, etc. */
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
  /* Update the modem information, and if we need to send information back
   * to the base, do so now.
   */
  if(kayak.modem) {
    modemUpdate(kayak.modem);
    if(modemNeedsPacket(kayak.modem)) {
      sendPacket();
    }
  }
}

void sendPacket()
{
  /* Packet structure corresponding to Santa Clara's format,
   * mostly just querying TinyGPS for information
   */
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
  /* Fill out the structure with the required data */
  /* Compass heading */
  if(kayak.compass)
    packet.heading = compassBearing(kayak.compass);
  int year;
  byte month, day, hour, minute, second;
  /* GPS time */
  kayak.gpsdata.crack_datetime(&year, &month, &day, &hour,
			       &minute, &second);
  packet.time = hour * 3600.0f + minute * 60.0f + second;
  /* GPS latitude and longitude */
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
  /* Misc. GPS information */
  packet.satellites = (byte)kayak.gpsdata.satellites();
  packet.hdilution = (short)kayak.gpsdata.hdop();
  packet.course = (short)kayak.gpsdata.f_course() * 100;
  packet.magcourse = (short)kayak.gpsdata.f_course() * 100;
  packet.groundspeed = (short)kayak.gpsdata.f_speed_kmph();
  /* For verification */
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
    /* Send the packet that we filled out */
    modemSendPacket(kayak.modem, &packet, sizeof(packet));
}

void enableTRNG(void)
{
  pmc_enable_periph_clk(ID_TRNG);
}

uint32_t trandom(void)
{
  static bool enabled = false;
  if(!enabled) {
    pmc_enable_periph_clk(ID_TRNG);
    TRNG->TRNG_IDR = 0xFFFFFFFF;
    TRNG->TRNG_CR = TRNG_CR_KEY(0x524e47) | TRNG_CR_ENABLE;
    enabled = true;
  }
  while (! (TRNG->TRNG_ISR & TRNG_ISR_DATRDY));
  return TRNG->TRNG_ODATA;
}
