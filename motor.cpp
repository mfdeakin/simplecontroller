
#include "motor.h"

/* Structure used to keep up with the state of the motor controller */
struct motorctrl
{
  /* The serial port the motor controller is connected to */
  USARTClass *serial;
  /* Whether or not the motor controller was detected */
  bool attached;
};

/* Checks whether the motor controller is attached
 * Preconditions: A valid motor controller, a positive timeout value
 * Postconditions: An up to date check for the motor controller,
 * 		   completed in no more than timeout ms
 */
bool motorCheckAttached(struct motorctrl *motor, int timeout);

/* Reads a byte from the motor controllers serial port.
 * Needed because the Arduino library doesn't allow the program
 * to specify the correct data format in serial initialization
 * Preconditions: A valid serial object, connected to the motor controller
 * Postconditions: A character sent by the motor controller
 */
char motorReadByte(USARTClass *serial);

/* Writes a string to the motor controllers serial port.
 * Needed because the Arduino library doesn't allow the program
 * to specify the correct data format in serial initialization
 * Preconditions: A valid serial port, a non null pointer to
 * 		  a zero terminated string
 * Postconditions: The string was written to the motor controller
 */
void motorWriteString(USARTClass *serial, const char *str);

/* Writes a byte to the motor controllers serial port.
 * Needed because the Arduino library doesn't allow the program
 * to specify the correct data format in serial initialization
 * Preconditions: A valid serial port, a byte to be sent
 * Postconditions: The byte was written to the motor controller in
 * 		   the valid format
 */
void motorWriteByte(USARTClass *serial, byte b);

/* Writes a set of bytes to the motor controllers serial port.
 * Needed because the Arduino library doesn't allow the program
 * to specify the correct data format in serial initialization
 * Preconditions: A valid serial port, a pointer to the bytes to be sent
 * Postconditions: The byte was written to the motor controller in
 * 		   the valid format
 */
void motorWriteBytes(USARTClass *serial, const void *bytes, size_t len);

struct motorctrl *motorInit(USARTClass *serial, int timeout)
{
  struct motorctrl *motor = (struct motorctrl *)malloc(sizeof(struct motorctrl));
  if(!motor)
    return NULL;
  memset(motor, 0, sizeof(struct motorctrl));
  /* The motor controller communicates at 9600 baud */
  motor->serial = serial;
  motor->serial->begin(9600);
  if(!motorCheckAttached(motor, timeout)) {
    free(motor);
    return NULL;
  }
  return motor;
}

void motorFree(struct motorctrl *motor)
{
  /* Turn off the motors, then free the memory */
  motorSetSpeed(motor, 0, 0);
  free(motor);
}

struct channelpair motorCheckAmp(struct motorctrl *motor)
{
  /* Motor Response Format:
   * ##\n##\n
   * Where the numbers are in hex and range from 0 to 7F
   * \n is a newline
   */
  char buffer[7];
  if(!motorWriteCmd(motor, "?a", buffer, sizeof(char[6]), 1000)) {
    DEBUGPRINT("Could not read amps!\r\n");
    return {0, 0};
  }
  struct channelpair values;
  memset(&values, 0, sizeof(values));
  buffer[6] = 0;
  DEBUGSERIAL.print("Values read:\r\n");
  DEBUGSERIAL.print(buffer);
  DEBUGSERIAL.print("\r\n");
  /* Convert the values to integers */
  int check = sscanf(buffer, "%xd\n%xd\n", &values.cA, &values.cB);
  DEBUGSERIAL.print("Read amp values: ");
  DEBUGSERIAL.print(check);
  DEBUGSERIAL.print(", ");
  DEBUGSERIAL.print(values.cA);
  DEBUGSERIAL.print(", ");
  DEBUGSERIAL.print(values.cB);
  DEBUGSERIAL.print("\r\n");
  return values;
}

struct channelpair motorCheckWatt(struct motorctrl *motor)
{
  /* Motor Response Format:
   * ## ##\r
   * Where the numbers are in hex and range from 0 to 7F
   * \r is a carriage return
   */
  char buffer[7];
  if(!motorWriteCmd(motor, "?v", buffer, sizeof(char[6]), 1000)) {
    DEBUGPRINT("Could not read voltages!\r\n");
    return {0, 0};
  }
  struct channelpair values;
  memset(&values, 0, sizeof(values));
  buffer[6] = 0;
  DEBUGSERIAL.print("Values read:\r\n");
  DEBUGSERIAL.print(buffer);
  DEBUGSERIAL.print("\r\n");
  /* Convert the values to integers */
  int check = sscanf(buffer, "%xd\n%xd\n", &values.cA, &values.cB);
  DEBUGSERIAL.print("Read volt values: ");
  DEBUGSERIAL.print(check);
  DEBUGSERIAL.print(", ");
  DEBUGSERIAL.print(values.cA);
  DEBUGSERIAL.print(", ");
  DEBUGSERIAL.print(values.cB);
  DEBUGSERIAL.print("\r\n");
  return values;
}

int motorWriteCmd(struct motorctrl *motor, const char *cmd,
		  void *buffer, size_t size, int timeout)
{
  /* Clear out the serial buffer, we don't want to worry about overflow */
  while(motor->serial->available())
    motor->serial->read();
  /* Write the comand to the motor controller, followed by an EOL so the
   * controller knows it's recieved an entire command 
   */
  motorWriteString(motor->serial, cmd);
  motorWriteString(motor->serial, "\r\n");
  motor->serial->flush();
  /* Read the output of the motor controller, but take no longer than
   * the timeout specified
   */
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
  /* The motor controller should echo the command back,
   * so find that to know that we're reading what we're looking for.
   * Typical state machine logic
   */
  for(i = 0;
      cmd[i] && delta * TICKSPERTENMS < timeout;
      delta = GetTickCount() - starttime) {
    char ret = motorReadByte(motor->serial);
    if(ret == '\r') {
      DEBUGPRINT("\r\n");
    }
    else {
      DEBUGPRINT(ret);
    }
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
    /* We couldn't find the command in the echo with the time provided,
     * so give up. (The number of characters read which matched the command
     * was less than the number of characters in the command
     */
    return -1;
  }
  byte *buf = (byte *)buffer;
  for(i = 0;
      i < size && delta * TICKSPERTENMS < timeout;
      i++, delta = GetTickCount() - starttime) {
    buf[i] = motorReadByte(motor->serial);
  }
  return i;
}

void motorSetSpeed(struct motorctrl *motor, float fwd, float rot)
{
  /* This is a simple command which doesn't require a response
   * from the motor controller, so just build it and run it.
   */
  int forward = fwd * 0x7F,
    rotation = rot * 0x7F;
  char cmdA = 'A',
    cmdB = 'B';
  /* Set channel A */
  if(forward < 0) {
    /* All values should be positive, the motor controller determines
     * sign based on the case of the command character
     */
    forward = -forward;
    cmdA = 'a';
  }
  char buffer[10];
  sprintf(buffer, "!%c%02X\r\n", cmdA, forward);
  DEBUGPRINT(buffer);
  motorWriteString(motor->serial, buffer);
  /* Set channel B */
  if(rotation < 0) {
    rotation = -rotation;
    cmdB = 'b';
  }
  sprintf(buffer, "!%c%02X\r\n", cmdB, rotation);
  DEBUGPRINT(buffer);
  motorWriteString(motor->serial, buffer);
  motor->serial->flush();
}

bool motorCheckAttached(struct motorctrl *motor, int timeout)
{
  /* Check for the motor controller
   * This is a little iffy, but the motor controller consistently responds
   * with OK if an indeterminant number of carriage returns are sent to it.
   * The number is not 10, which one might suspect given the documentation
   * on how to bring the motor controller into serial command.
   */
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
    DEBUGSERIAL.print("Checking for motor controller connection\r\n");
    for(int i = 0; i < 20; i++)
      motorWriteByte(motor->serial, '\r');
    /* Give the motor controller a chance to respond, wait 50 ms */
    delay(50);
    timeout -= 50;
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
  /* Just strip off the parity bit */
  return serial->read() & 0x7f;
}

void motorWriteString(USARTClass *serial, const char *str)
{
  /* Just iterate over the strings bytes, and send those.
   * Don't send the null terminator, though. Let the user do that
   * in the odd instance that they need to.
   */
  for(int i = 0; str[i]; i++) {
    motorWriteByte(serial, str[i]);
  }
}

void motorWriteBytes(USARTClass *serial, const void *bytes, size_t len)
{
  /* Write the bytes to the motor controller by iterating over them
   * and sending them individually.
   */
  byte *b = (byte *)bytes;
  for(int i = 0; i < len; i++) {
    motorWriteByte(serial, b[i]);
  }
}

void motorWriteByte(USARTClass *serial, byte b)
{
  /* The motor controller expects the following format:
   * pxxx xxxx
   * Where p is the parity bit.
   * To build this, remove the 7th bit, the determine the parity of the
   * remaining bits. Finally, add in the parity bit.
   */
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
