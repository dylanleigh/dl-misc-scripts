
"""
streamstatus.py Dylan Leigh 2016
Prints status messages at a visible speed, like an old teletype system.

Intended for use as a retro text-stream dashboard equivalent - either
on its own from a terminal, or as a module providing a
continuously-printing element within a more elaborate curses or GUI
dashboard.

CONFIGURATION:

Formatters - These are modular objects which return an output string
to be displayed by the main program. Some example ones are provided
which output the current time (TimeFormatter), arbitrary command
output (*SubProcFormatter), System information etc.

The app starts by calling display_loop() with a list of formatters
which run in sequence. If run as __main__, the app is set up to run a
bunch of example formatters (see the FORMATTERS var).
"""

from time import sleep, ctime
from sys import stdout
from os import uname
from subprocess import CalledProcessError, Popen, PIPE


# TODO these should be cli args
DEFAULT_SLEEP_TIME = 0.5
DEFAULT_CHAR_TIME = 0.03

class BaseFormatter(object):
   source = None
   def __init__(self, header=None):
      self.header = header
   def get_output(self):
      raise NotImplementedError

class TimeFormatter(BaseFormatter):
   def get_output(self):
      return ctime()

class UnameFormatter(BaseFormatter):
   def get_output(self):
      return '  '.join(uname())

class RepeatSubProcFormatter(BaseFormatter):
   """Uses output of command running in subprocess, which runs and exits"""
   def __init__(self, command_arg_list, header=None):
      self.header = header
      self.command_arg_list = command_arg_list
   def get_output(self):
      try:
         return Popen(self.command_arg_list, stdout=PIPE, text=True).communicate()[0]
      except OSError as ose:
         return 'ERROR: %s raised OSError: %s' % (
            self.command_arg_list[0],
            ose.strerror,
         )
      except CalledProcessError as cpe:
         return 'ERROR: %s returned %d: %s' % (
            self.command_arg_list[0],
            cpe.returncode,
            cpe.output,
         )

class ContinuousSubProcFormatter(BaseFormatter):
   """Uses output of command running in subprocess, which runs continuously"""
   # TODO keep subproc open in bg
   # TODO flag - display all since last output, or just last line?
   pass  # TODO

class SysInfoFormatter(BaseFormatter):
   """Gets CPU, memory, uptime, etc info from /proc/ instead of
      spawning a subprocess"""
   # TODO use background task - open file and reprocess?
   pass  # TODO
# TODO move above to module

# Example output config =============================================

EXAMPLE_F_TIME = TimeFormatter(header='CURRENT TIME:')
EXAMPLE_F_COMMAND_LINE = RepeatSubProcFormatter([
   'rastodo',
   '--days=1',
   '--ex-types=w',
   ],
   header='URGENT TODO ITEMS:',
)
FORMATTERS = [
    EXAMPLE_F_TIME,
    UnameFormatter(),
    RepeatSubProcFormatter(['uptime']),
    EXAMPLE_F_TIME,
    EXAMPLE_F_COMMAND_LINE,
]

# Main display loop code ============================================

def slow_write(string, char_time=DEFAULT_CHAR_TIME):
   # TODO: Option for colorizing (similar to lolcat)?
   for char in string:
      stdout.write(char)
      stdout.flush()
      sleep(char_time)

def display_loop(formatters,
                sleep_time=DEFAULT_SLEEP_TIME, char_time=DEFAULT_CHAR_TIME):
   if not formatters:
      return 'Error: Need at least one formatter!'
   while True:
      # TODO allow for randomness or other patterns?
      for f in formatters:
         if f.header:
            slow_write(f.header)
            stdout.write('\n')
         slow_write(f.get_output())

         # clear and wait for next line(s)
         stdout.write('\n')
         sleep(sleep_time/2)
         stdout.write('\n')
         sleep(sleep_time/2)
   # end of main while true loop


# MAIN STARTS HERE ==================================================
if __name__ == "__main__":
   try:
      # TODO any sources required by formatters need to be set up
      # before display loop starts
      print(display_loop(FORMATTERS))
   except KeyboardInterrupt:
      pass

