
#include <Arduino.h>
#include "include.h"
#include "list.h"
#include "modem.h"
#include "motor.h"

void initMotorController(void);

struct kayak {
  struct modem *modem;
  struct motorctrl *motor;
  float forwardpwr, rotationpwr;
  unsigned powerused;
} kayak;

void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);
char *readLine(HardwareSerial *sio);

void setup(void)
{
  DEBUGSERIAL.begin(9600);
  for(int i = 0;; i++) {
    DEBUGSERIAL.print(i);
    DEBUGSERIAL.println(" Hello, there!");
    DEBUGSERIAL.flush();
    delay(100);
  }
  kayak.modem = modemInit(&MODEMSERIAL, 5000);
  if(!kayak.modem) {
    DEBUGSERIAL.print("Modem not connected\r\n");
  }
  else {
    DEBUGSERIAL.print("Modem connected\r\n");
  }
  kayak.motor = motorInit(&MOTORSERIAL, 1000);
  if(!kayak.motor) {
    DEBUGSERIAL.print("Motor controller not connected\r\n");
  }
  else {
    DEBUGSERIAL.print("Motor controller connected\r\n");
  }
  kayak.forwardpwr = 0.0f;
  kayak.rotationpwr = 0.0f;
  startTimer(TC1, 0, TC3_IRQn, 4);
}

void loop()
{
  /* Hack for testing */
  kayak.forwardpwr = modemReadFloat(kayak.modem, 100);
  kayak.rotationpwr = modemReadFloat(kayak.modem, 100);
  motorSetSpeed(kayak.motor, kayak.forwardpwr, kayak.rotationpwr);
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
