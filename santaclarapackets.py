#!python
import sys, serial, platform, pygtk, gtk, gobject
from numpy import frombuffer, float16

keyvals = {
    65362: "Up",
    65364: "Down",
    65361: "Left",
    65363: "Right"
}

class Kayak:
    def __init__(self):
        self.keystates = {
            "Up": False,
            "Down": False,
            "Right": False,
            "Left": False
        }
        
        self.timerId = gobject.timeout_add(1000, self.timerEvent)
        
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.connect("delete_event", self.deleteEvent)
        self.window.connect("destroy", self.destroy)
        self.window.connect("key_press_event", self.keyPress)
        self.window.connect("key_release_event", self.keyRelease)
        self.window.show()
        
        self.serial = serial.Serial("/dev/ttyACM1", 19200)
    
    def keyPress(self, widget, event, data = None):
        if event.keyval in keyvals:
            self.keystates[keyvals[event.keyval]] = True
        return True
    
    def keyRelease(self, widget, event, data = None):
        if event.keyval in keyvals:
            self.keystates[keyvals[event.keyval]] = False
        return True
    
    def timerEvent(self):
        print("Timer event!")
        chA = 0
        if self.keystates["Up"]:
            chA += 0.5
        elif self.keystates["Down"]:
            chA -= 0.5
        self.serial.write(floattochar(chA))
        
        chB = 0
        if self.keystates["Left"]:
            chB += 0.5
        elif self.keystates["Right"]:
            chB -= 0.5
        self.serial.write(floattochar(chB))
        print("Channel A Value: " + str(chA))
        print("Channel B Value: " + str(chB))
        while self.serial.inWaiting() > 0:
            sys.stdout.write(self.serial.read())
        return True
    
    def deleteEvent(self, widget, event, data = None):
        print("Delete event " + str(event))
        return False
    
    def destroy(self, widget, data = None):
        print("Destroy event")
        gtk.main_quit()
    
    def main(self):
        gtk.main()
    

def floattochar(f):
    if f == 0.0:
        return chr(0) + chr(0)
    fhexstr = float.hex(f)
    if fhexstr[0] == '-':
        negative = True
        fhexstr = fhexstr[1:]
    else:
        negative = False
    mantissa = fhexstr[4:17]
    exponent = (int(fhexstr[18:]) + 15) << 10
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
    byte = bytearray(2)
    byte[1] = chr(byte1)
    byte[0] = chr(byte2)
    print("Byte 0: " + str(byte[0]))
    print("Byte 1: " + str(byte[1]))
    print("Numpy result: " +
          str(frombuffer(byte, dtype=float16)[0]))
    return byte

def inttochar(val):
    byte = bytearray(2)
    byte[0] = (val & 0xFF00) >> 8
    byte[1] = val & 0xFF
    return byte

wnd = Kayak()
wnd.main()

exit()

s = serial.Serial("/dev/ttyACM1", 19200)

try:
    while True:
        cmd = raw_input()
        try:
            val = float(cmd)
            cmd = floattochar(val)
        except ValueError:
            print("Not a floating point value")
            if cmd != "+++":
                cmd += "\r\n"
        print("Sending command: " + str(cmd))
        print("Sent " + str(s.write(cmd)) + " bytes")
        while s.inWaiting() > 0:
            sys.stdout.write(s.read())
except KeyboardInterrupt, EOFError:
    print("Closing serial port")
    s.close()
