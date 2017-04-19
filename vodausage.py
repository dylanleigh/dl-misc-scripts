#!/usr/bin/env python
#
# vodausage - Parses postpaid Vodafone usage data and tells you how
# much data and calls you have used.
#
# The vodafone site used to display this incorrectly (hence this 
# script); the site has recently been fixed.
#
# You need to log in to "My Vodafone", and go to the usage details
# page then download the data. It should give you a CSV file - run
# this script with the file as and argument.
#
# --=--
#
# Copyright (c) 2011 Dylan Leigh.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the above copyright notice,
# this list of conditions and the following disclaimer are retained.
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
# --=--
#


import os, sys, csv

# This is the minimum size of a session in KB
MINSESSIONDATA=15


if (len(sys.argv) < 2):
   sys.exit("Usage: vodausage.py <calls-usage-file.csv>")

csvfile = csv.reader(open(sys.argv[1], 'rb'))

totdata = 0.0        # Data actually used
totdatarounded = 0.0 # Data, each session rounded to minimum
totcalls = 0.0       # All non-data in $ (calls, sms)
charges = 0.0        # All extra charges not covered by plan

csvfile.next() # Skip the title line
for row in csvfile:
   #debug print row

   # is packet data?
   if (row[2]=="PKT"):
      # row[3] is of the form "Internet xxxxKb" so we rm the text
      data = int(row[3].lstrip("Internet ").rstrip("Kb"))
      #print row[3]
      #debug print data

      totdata += data
      # minimum session size
      totdatarounded += max(data,MINSESSIONDATA)
   else:
      # debug print float(row[7].lstrip("$"))
      # calls/sms/misc. Add to usage
      totcalls += float(row[7].lstrip("$"))

   # sum extra charges
   charges += float(row[8].lstrip("$"))

print "Calls/SMS: $ %s   Extra Charges: $ %s"%(totcalls,charges)
print "Data (unrounded): %s Mb   Data (billed): %s Mb"%\
      (totdata/1000,totdatarounded/1000)

