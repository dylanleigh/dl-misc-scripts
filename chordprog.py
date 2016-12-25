#!/usr/local/bin/python

# Chord Progression Generator - Dylan Leigh 2016



# The Plan:
#           - We start with the root chord I at the "end" of the
#             progression, and work backwards until we get to another I.
#           - Each chord of the scale (I-vii) has a before_*() method, which
#             picks from a set of candidate chords which could go before it.

# TODO: Handle the kludge of "viio" for diminished, use the symbol?
# TODO: Min/Max arguments to generate, pass in from cli
# TODO: Other scales - minors, pentatonic, blues, chromatic?
# TODO: Incorporate non-scale chords
# TODO: Incorporate altered root chords

import random

class Progression:
   def __init__(self):
      self.chords = ['I']    # Work backwards from I

   def __str__(self):
      return str(self.chords)

   def before_I(self):
      path = random.randint(0,1)
      if path == 1:
         return random.choice(['IV','V','viio'])
      else:
         return random.choice(['IV','V'])

   def before_ii(self):
      return random.choice(['I','iii','IV','vi'])

   def before_iii(self):
      return random.choice(['I','ii','IV'])

   def before_IV(self):
      return random.choice(['I','iii','vi'])

   def before_V(self):
      return random.choice(['I','ii','IV','vi'])

   def before_vi(self):
      return random.choice(['I','iii','V'])

   def before_viio(self):
      return random.choice(['I','ii','IV'])

   def generate(self):
      curr = self.chords[0]
      while len(self.chords) < 2 or curr != 'I':
         func = getattr(self, "before_%s" % curr)
         curr = func()
         self.chords.insert(0, curr)

         # NOTE: Before this was a class we could still do this with
         # attributed of the current module:
         # getattr(sys.modules[__name__], "before_%s" % curr)()

if __name__ == '__main__':
   prog = Progression()
   prog.generate()
   print prog
