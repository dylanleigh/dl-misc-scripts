#!/usr/bin/env python
#
# Generate a set of basic waveform (Sine, Triangle, Saw, Square...)
# samples for chiptunes etc


import os,sys,csv,time,re


class WaveForm(object):
   '''Abstract class for all forms'''

   def write(self):
      f = open(self.filename, 'w')
      f.write(bytearray(self.data))
      f.close()


class SawTooth(WaveForm):

   def __init__(self, filename='sawtooth.raw'):
      self.filename = filename
      self.data = range(256)


class Triangle(WaveForm):

   def __init__(self, filename='triangle.raw'):
      self.filename = filename
      self.data = range(0,256,2)+range(255,0,-2)

class Square(WaveForm):
   '''Duty cycle between 0-1, default 0.5'''

   def __init__(self, filename='square.raw', duty=0.5):
      self.filename = filename
      self.data = [0]*int(256*duty) + [255]*int(256*(1-duty))

if __name__ == '__main__':
   saw = SawTooth()
   saw.write()
   tri = Triangle()
   tri.write()
   square = Square()
   square.write()
   quart = Square(filename='quarter.raw', duty=0.25)
   quart.write()
