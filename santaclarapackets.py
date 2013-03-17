import sys, serial, platform

def floattochar(f):
    if f == 0.0:
        return chr(0) + chr(0)
    fhexstr = float.hex(f)
    if fhexstr[0] == '-':
        negative = True
        fhexstr[1:]
    else:
        negative = False
    mantissa = fhexstr[4:17]
    exponent = (int(fhexstr[19:]) + 15) << 10
    fraction = int(mantissa[:3], 16) >> 2
    finalval = exponent + fraction
    if negative:
        finalval |= 1 << 15
    byte2 = finalval % (1 << 8)
    byte1 = (finalval - byte2) >> 8
    print("fhexstr: " + fhexstr + "\nMantissa: " + mantissa +
          "\nExponent: " + str(exponent) + "\nFraction: " + str(fraction) +
          "\nFinal value: " + str(finalval) +
          "\nByte 1: " + hex(byte1) + "\nByte 2: " + hex(byte2))
    return chr(byte1) + chr(byte2)

s = serial.Serial("/dev/ttyACM1", 19200)

try:
    while True:
        cmd = raw_input()
        try:
            fval = float(cmd)
            cmd = floattochar(fval)
        except ValueError:
            print("Not a floating point value")
            if cmd != "+++":
                cmd += "\r\n"
        print("Sending command: " + cmd)
        s.write(cmd)
        while s.inWaiting() > 0:
            sys.stdout.write(s.read())
except KeyboardInterrupt, TypeError:
    print("Closing serial port")
    s.close()

