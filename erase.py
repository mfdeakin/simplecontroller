#!/usr/bin/python
import serial
import sys

s = serial.Serial("/dev/" + sys.argv[1], 1200)
s.open()
s.close()
