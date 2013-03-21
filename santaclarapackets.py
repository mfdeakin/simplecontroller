#!python
import sys, serial, platform, pygtk, gtk, gobject

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
        
        self.timerId = gobject.timeout_add(10, self.timerEvent)
        
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.connect("delete_event", self.deleteEvent)
        self.window.connect("destroy", self.destroy)
        self.window.connect("key_press_event", self.keyPress)
        self.window.connect("key_release_event", self.keyRelease)
        self.window.show()
        
        #self.serial = serial.Serial("/dev/ttyACM1", 19200)
    
    def keyPress(self, widget, event, data = None):
        print("Key pressed " + str(event.keyval))
        if event.keyval in keyvals:
            self.keystates[keyvals[event.keyval]] = True
        return True
    
    def keyRelease(self, widget, event, data = None):
        print("Key released " + str(event.keyval))
        if event.keyval in keyvals:
            self.keystates[keyvals[event.keyval]] = False
        return True
    
    def timerEvent(self):
        print("Timer event!")
        chA = 0
        if self.keystates["Up"]:
            chA += 127
        elif self.keystates["Down"]:
            chA -= 127
        #self.serial.write(inttochar(chA))
        
        chB = 0
        if self.keystates["Left"]:
            chB += 127
        elif self.keystates["Right"]:
            chB -= 127
        #self.serial.write(inttochar(chB))
        #while self.serial.inWaiting() > 0:
            #sys.stdout.write(self.serial.read())
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
    byte = bytearray(2)
    byte[0] = chr(byte1)
    byte[1] = chr(byte2)
    print("Byte 0: " + str(byte[0]))
    print("Byte 1: " + str(byte[1]))
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
            val = int(cmd)
            cmd = inttochar(val)
        except ValueError:
            print("Not a floating point value")
            if cmd != "+++":
                cmd += "\r\n"
        print("Sending commend: " + str(cmd))
        print("Sent " + str(s.write(cmd)) + " bytes")
        while s.inWaiting() > 0:
            sys.stdout.write(s.read())
except KeyboardInterrupt, TypeError:
    print("Closing serial port")
    s.close()
