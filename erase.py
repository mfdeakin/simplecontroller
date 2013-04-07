#!/usr/bin/python
import serial
import sys

s = serial.Serial("/dev/" + sys.argv[1], 1200)
if not s.isOpen():
    s.open()
s.close()
