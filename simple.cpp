
#include <Arduino.h>
#include "include.h"
#include "list.h"
#include "modem.h"
#include "motor.h"

void initMotorController(void);

struct kayak {
  struct modem *modem;
  struct motorctrl *motor;
  unsigned powerused;
} kayak;

char *readLine(HardwareSerial *sio);

void setup(void)
{
  DEBUGSERIAL.begin(9600);
  DEBUGPRINT("Attempting to connect the modem\r\n");
  kayak.modem = modemInit(&MODEMSERIAL, 5000);
  if(!kayak.modem) {
    DEBUGSERIAL.print("Modem not connected\r\n");
  }
  else {
    DEBUGSERIAL.print("Modem connected\r\n");
  }
  DEBUGPRINT("Attempting to connect the motor controller\r\n");
  kayak.motor = motorInit(&MOTORSERIAL, 1000);
  if(!kayak.motor) {
    DEBUGSERIAL.print("Motor controller not connected\r\n");
  }
  else {
    DEBUGSERIAL.print("Motor controller connected\r\n");
  }
}

void loop()
{
  delay(500);
  /* Hack for testing */
  if(kayak.motor) {
    motorCheckVolt(kayak.motor);
    motorCheckAmp(kayak.motor);
    modemUpdate(kayak.modem);
  }
  if(kayak.modem && modemHasPacket(kayak.modem) && kayak.motor) {
    motorSetSpeed(kayak.motor, modemForwardPwr(kayak.modem), modemRotationPwr(kayak.modem));
  }
}

// void loop(void)
// {
//   const int BUFSIZE = 100;
//   char buffer[BUFSIZE];
//   char nil = 0; //Protective byte
//   if(MODEMSERIAL.available() > 0) {
//     if(!connected) {
//       connected = true;
//       for(int i = 0; i < MODEMSERIAL.available(); i++) {
//       /* I think this works... */
//         if(MODEMSERIAL.read() != "CONNECTED"[i]) {
//           connected = false;
//           break;
//         }
//       }
//       if(connected) {
//         DEBUGSERIAL.println("Modems connected");
//       }
//     }
//     /* Read until we fill up the buffer or we hit a carriage return */
//     buffer[0] = MODEMSERIAL.read();
//     int i;
//     for(i = 1;
//         i < BUFSIZE && buffer[i - 1] != '\r';
//         i++) {
//       buffer[i] = MODEMSERIAL.read();
//     }
//     buffer[i] = 0;
//     /* Determine if we are to change the power sent to a motor */
//     int number;
//     sscanf(buffer, "%d", &number);
//     if(buffer[i - 1] == 'r')
//       rightmotor = number & 0x7f;
//     else if(buffer[i - 1] == 'l')
//       leftmotor = number & 0x7f;
//   }
// }
