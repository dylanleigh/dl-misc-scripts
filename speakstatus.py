"""
speakstatus.py - 2016 Dylan Leigh

This is a script designed to speak "status" info such as the
time/public transport/appointments/etc etc. It is intended to be
periodically run from cron or similarly triggered such as at login and
uses espeak in a subprocess to do the actual speaking.

CONFIGURATION:
There are a set of "Sources" which generate output
to be spoken. These are put in a global called SOURCES and the
script says their output sleeping a bit between each.

TODO:
- command line config
- config file config
- more sources
   - PTV API
   - Google calendar
   - Proper rastatodo integration
- more natural time phrasing
"""

from time import sleep, strftime
from subprocess import check_output, check_call


DEFAULT_SLEEP_TIME=1

class BaseSource(object):
   """
   Source ABC, takes an optional "prefix" argument to say a
   fixed string before the main content.
   """
   def __init__(self, prefix=None):
      self.prefix = prefix
   def speech(self):
      raise NotImplementedError

class TimeSource(BaseSource):
   """
   Says the hour and minute rather mechanically
   """
   def speech(self):
      return strftime('%H %M')

class SubProcSource(BaseSource):
   """
   Wraps and returns output of a subprocess
   """
   def __init__(self, command_arg_list, prefix=None):
      self.prefix = prefix
      self.command_arg_list = command_arg_list

   def get_stdout(self):
      return check_output(self.command_arg_list)
      # NOTE: May throw CPE or OSErrors

   def speech(self):
      return self.get_stdout()

class CountSubProcSource(SubProcSource):
   """
   Returns the number of lines output by the subprocess
   """
   def speech(self):
      output = self.get_stdout()
      return str(output.count('\n'))


# EXAMPLE OUTPUT CONFIG ===================================

SOURCES = [
   TimeSource(prefix="Time:"),

   # Says how many items due today in rastodo
   CountSubProcSource(
      ['rastodo', '--days=0', '--ex-types=w'],
      prefix="To Do Today:"
   )
]

# ESPEAK CLASS ============================================

class ESpeak(object):
   """
   Says strings through a subprocess running espeak
   """
   def __init___(self):
      pass # TODO: don't be lazy and keep spawning new ones, use -stdin arg
   def say(self, string):
      return check_output(['espeak', string])


# MAIN ====================================================

if __name__ == "__main__":
   espeak = ESpeak()
   for src in SOURCES:
      if src.prefix:
         espeak.say(src.prefix)
      espeak.say(src.speech())
      sleep(DEFAULT_SLEEP_TIME)
