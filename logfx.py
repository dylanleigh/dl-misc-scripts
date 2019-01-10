#!/usr/bin/env python
#
# Logfx.py - Sound Effects for your logs
#
# Monitors a number of files for new lines; when a line is appended
# matching a specified regular expression, play a specified sound file.
#
# Also accepts expressions with a countdown timer (in seconds)
# embedded (e.g. "You have to wait 47 seconds for the next attempt".
# When this option is in effect the sound will be played after
# a delay of that many seconds.
#
# Requires pygame to play actual sounds (uses the mixer module to play audio).
# Otherwise it will fall back to sending beeps to the terminal for
# each event.
#
# Some assorted interesting, fun and mundane uses:
#
#  - Immediate audio alert of all sorts of serious problems from
#    syslog or dmesg.
#
#  - Use with a very short & soft click sound to create a "gieger counter"
#    effect; you can set this on a log file with a very widely matching regex
#    (even ".") to tell when you are getting a lot of messages. Particularly fun
#    with mail or web server logs.
#
#  - Monitor dmesg or syslog for new wifi networks or transient errors.
#
#  - MMORPGs (and other games) where you might switch away to another app while
#    you are mining/travelling/doing something else which will take a few
#    minutes. You can set sound effects to alert you noisily if you are being
#    attacked or you can't do any more mining.
#
# Config file format is just a CSV file with each line of the form:
#
# <file-to-watch>,<regex>,<file-to-play>[,<options>]
#
# Options:
# "noecho" - Don't print the line to the terminal
# "delay" - requires that the regex include ([:digit:]) or similar to
# match the number of seconds to delay the sound effect (see example below).
#
# e.g.:
# /var/log/messages,device no longer idle,/home/me/soundfx/ping.wav
# /var/log/messages,Found new beacon,/home/me/soundfx/ping.wav,noecho
# /var/log/messages,Wait for ([0-9]+) sec,/home/me/soundfx/ping.wav,delay
#
# -----
#
# Copyright (c) 2012 Dylan Leigh.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#
# -----
#

# TODO:
#        - Option to play a sound effect on any exceptions which stop
#          the script?

import os,sys,csv,time,re
try:
   import pygame
except (ImportError):
    print "Pygame not found, will use terminal beep instead"
    pygame = False


################################################# globals

debug=False

logFiles = dict()    # logfilepath -> WatchFile object
if pygame:
   soundFiles = dict()  # soundfilepath -> pygame sound object
delayQueue = list()  # tuple (time_t to trigger, soundfile index)

################################################# classes

class WatchFile (object):
   '''Encapsulates everything about a individual file that is watched
      for changes - file name, file object, list of regexes and the
      name of the sound file to play if there is a match.'''

   def __init__(self, filename):
      self.filename = filename
      self.fobj = open(filename)
      self.fobj.seek(0,os.SEEK_END) # skip to end of file
      self.matchList = list() # new empty list of matches
      if (debug):
         print ('Watching file %s'%(filename,))

   def addMatch(self, regex, soundfile, options):
      '''Add a match to the list for this file. Pass the regex in text
         form; it is stored compiled'''
      tup = (re.compile(regex), soundfile, options)
      self.matchList.append(tup)
      if (debug):
         print ('Adding match %s to file %s, playing %s'%(regex,self.filename,soundfile))

   def findMatches(self):
      '''Looks for new matches, returns a list of soundfiles to play'''
      ret = list()

      # reset EOF condition

      # Read each line, see if it matches each regex
      for line in self.fobj:
         for tup in self.matchList:
            match = tup[0].search(line)
            if (match):
               # matched the regex - play effect etc
               options = tup[2]
               if (options):
                  if ("delay" in options):
                     # get num of seconds
                     sec = match.group(1)
                     # determine time in future to play
                     future = time.time() + float(sec)
                     # add to queue
                     delayQueue.append((future, tup[1]))
                     continue;
                  if ("delay" not in options):
                     # play straight away
                     if pygame:                    # TODO DRY
                        soundFiles[tup[1]].play()
                     else:
                        print ""
                  if ("noecho" not in options):
                     print "%s:%s"%(self.filename,line)
               else:
                  # no options - default
                  print "%s:%s"%(self.filename,line)
                  if pygame:                    # TODO DRY
                     soundFiles[tup[1]].play()
                  else:
                     print ""

      self.fobj.seek(0,os.SEEK_CUR) # XXX: hack to clear internal for loop buffer
      # end finMAtches
# end WatchFile

################################################# main begins here

if pygame:
   pygame.mixer.init()

# open config file as commandline argument or exit
if (len(sys.argv) < 2):
   sys.exit("Usage: logfx.py <config-file.csv>")

configcsv = csv.reader(open(sys.argv[1], 'rb'))

### parse config file into global structures
### use dicts to prevent repeat of logfile or soundfile loading

for row in configcsv:
   if (len(row) < 3):
      # assert row >= 3 items
      print "Row too small: %s"%row
      continue;

   # 3 or 4 per line
   logf = row[0]
   regex = row[1]
   sfile = row[2]
   if (len(row) > 3):
      options = row[3:] # XXX: options is a List
   else:
      options = None

   if (not logf in logFiles):       # logfile not referenced before
      logFiles[logf] = WatchFile(logf)
   # add new regex to new or existing entry
   logFiles[logf].addMatch(regex,sfile,options)

   if (pygame and not (sfile in soundFiles)):  # sound file not referenced before
      soundFiles[sfile] = pygame.mixer.Sound(sfile)
      soundFiles[sfile].set_volume(0.5) # TODO an option to set volume! 0-1.0


# end config file parsing

# main loop
while True:
   time.sleep(1)
   # scan for new lines
   for logf in logFiles:
      logFiles[logf].findMatches()
   # process delay queue
   for t in delayQueue:
      now = time.time()
      if (t[0]<=now): # time to trigger <= now
         if (pygame):                     # TODO DRY
            soundFiles[t[1]].play()
         else:
            print ""
         delayQueue.remove(t) # remove from queue
