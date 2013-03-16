
#include "motor.h"

struct motorctrl
{
  USARTClass *serial;
  bool attached;
};

bool motorCheckAttached(struct motorctrl *motor, int timeout);

char motorReadByte(USARTClass *serial);
void motorWriteString(USARTClass *serial, const char *str);
void motorWriteByte(USARTClass *serial, byte b);

struct motorctrl *motorInit(USARTClass *serial, int timeout)
{
  struct motorctrl *motor = (struct motorctrl *)malloc(sizeof(struct motorctrl));
  if(!motor)
    return NULL;
  motor->serial = serial;
  motor->serial->begin(9600);
  if(!motorCheckAttached(motor, timeout)) {
    free(motor);
    return NULL;
  }
  /* Set the motor controller to mixed mode.
   * Before we do that, read the current mode in so we don't change things unnecessarily
   */
  return motor;
}

void motorFree(struct motorctrl *motor)
{
  free(motor);
}

void motorSetSpeed(float forward, float rotate)
{
  
}

struct channelpair motorCheckAmp(struct motorctrl *motor)
{
  DEBUGSERIAL.print("Reading motor amperage\r\n");
  char buffer[6];
  motorWriteCmd(motor, "?a\n", buffer, sizeof(buffer), 10);
  struct channelpair values;
  memset(&values, 0, sizeof(values));
  int check = sscanf(buffer, "%d\n%d\n", &values.cA, &values.cB);
  DEBUGSERIAL.print("Read amp values: ");
  DEBUGSERIAL.print(check);
  DEBUGSERIAL.print(", ");
  DEBUGSERIAL.print(values.cA);
  DEBUGSERIAL.print(", ");
  DEBUGSERIAL.print(values.cB);
  DEBUGSERIAL.print("\r\n");
  DEBUGSERIAL.flush();
  return values;
}

struct channelpair motorCheckVolt(struct motorctrl *motor)
{
  DEBUGSERIAL.print("Reading motor voltages\r\n");
  char buffer[6];
  motorWriteCmd(motor, "?v\n", buffer, sizeof(buffer), 10);
  struct channelpair values;
  memset(&values, 0, sizeof(values));
  int check = sscanf(buffer, "%d\n%d\n", &values.cA, &values.cB);
  DEBUGSERIAL.print("Read volt values: ");
  DEBUGSERIAL.print(check);
  DEBUGSERIAL.print(", ");
  DEBUGSERIAL.print(values.cA);
  DEBUGSERIAL.print(", ");
  DEBUGSERIAL.print(values.cB);
  DEBUGSERIAL.print("\r\n");
  DEBUGSERIAL.flush();
  return values;
}

bool motorWriteCmd(struct motorctrl *motor, const char *cmd,
		   void *buffer, size_t size, int timeout)
{
  motorWriteString(motor->serial, cmd);
  motor->serial->flush();
  motor->serial->setTimeout(timeout);
  if(timeout > 0) {
    if(timeout > 10)
      timeout /= 10;
    else
      timeout = 1;
  }
  unsigned starttime = GetTickCount(),
    delta = GetTickCount() - starttime,
    i;
  for(i = 0;
      cmd[i] && delta * TICKSPERTENMS < timeout;
      delta = GetTickCount() - starttime) {
    char ret = motorReadByte(motor->serial);
    if(ret == cmd[i]) {
      i++;
    }
    else if(ret == cmd[0]) {
      i = 1;
    }
    else {
      i = 0;
    }
  }
  if(cmd[i]) {
    return false;
  }
  byte *buf = (byte *)buffer;
  for(i = 0;
      i < size && delta * TICKSPERTENMS < timeout;
      i++, delta = GetTickCount() - starttime) {
    buf[i] = motorReadByte(motor->serial);
  }
  if(i < size)
  return true;
}

bool motorCheckAttached(struct motorctrl *motor, int timeout)
{
  const char *states = "OK";
  const int statelen = 2;
  /* The motor needs to be sent 10 carriage returns before it
   * can accept commands.
   * After recieving it, it will respond with OK signifying that it is ready.
   */
  int inputstate = 0;
  timeout *= 10;
  DEBUGSERIAL.print("Connecting motor controller\r\n");
  for(; timeout > 0 && inputstate < statelen;) {
    for(int i = 0; i < 20; i++)
      motorWriteByte(motor->serial, '\r');
    /* Give the motor controller a chance to respond, wait 100 ms */
    delay(100);
    timeout -= 100;
    if(motor->serial->available() > 0) {
      while(motor->serial->available() > 0 && inputstate < statelen) {
	char check = motorReadByte(motor->serial);
	if(check == states[inputstate]) {
	  inputstate++;
	}
	else {
	  if(check == states[0]) {
	    inputstate = 1;
	  }
	  else {
	    inputstate = 0;
	  }
	}
      }
    }
  }
  if(inputstate == statelen) {
    motor->attached = true;
    return true;
  }
  else {
    motor->attached = false;
    return false;
  }
}

/* In the future, we should definitely get rid of the Arduino libraries.
 * The SAM libraries that come with the Arduino software can configure the
 * microcontroller to use 7 bits of data and even parity, with 1 stop bit.
 *
 * WARNING: Arduino library currently is set up to use a ring buffer
 * limited to 64 (they don't tell you that) bytes of data.
 * This is fine - if you're reading the data constantly and it's not being
 * sent too quickly. The motor controller, however, is constantly sending data,
 * and if not flushed quickly enough, important data may be lost.
 */
char motorReadByte(USARTClass *serial)
{
  return serial->read() & 0x7f;
}

void motorWriteString(USARTClass *serial, const char *str)
{
  for(int i = 0; str[i]; i++) {
    motorWriteByte(serial, str[i]);
  }
}

void motorWriteByte(USARTClass *serial, byte b)
{
  b <<= 1;
  byte parity = 0;
  byte temp = b;
  while(temp) {
    parity ^= temp & 0x80;
    temp <<= 1;
  }
  b = (b >> 1) | parity;
  serial->write(b);
}
