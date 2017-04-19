#!/usr/local/bin/perl
# $Id: dumps.pl 92 2009-11-20 01:36:27Z dleigh $
#
# Takes backup dumps of a set of filesystems, compresses them, and
# saves them to a specified location (default is /scratch/dumps).
#
# Finally, checks the dumps using restore -t.
#
# WARNING: This script was designed for FreeBSD 6 systems. The
# semantics of your system's dump command may vary. In particular,
# many other systems DO NOT support dumps of live read-write
# filesystems.
#
# Options that should be changed (or at least checked) by the user are
# marked with a "FIXME" nearby.
# 
# -----
#
# Copyright (c) 2006 Dylan Leigh.
#
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
# 3. Modified versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
# 4. The names of the authors or copyright holders must not be used to
#    endorse or promote products derived from this software without
#    prior written permission.
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

# USER: FIXME choose the desired schedule
use constant SCHEDULE => "W0INC";
# W0 - just a level 0 every sunday
# W0D1 - level 0 every sunday, level 1 on other days
# W0INC - level 0 sunday, level 1 monday, 2 tuesday... 6 saturday
# 7TOH - 7 Day TOH
# 14stp3 - 14 day 3 step TODO - requires reading dumpdates :(
# MWTOH - Monthly 0, Weekly TOH (starting level 1 on Sunday)
# MWINC - Monthly 0, Weekly Incremental (starting Sunday)

use strict;
use warnings;
use POSIX qw(strftime);

# USER: FIXME list the filesystems, output directory here
my %fstodump = ("root"=>"/", "usr"=>"/usr", "var"=>"/var");

# USER: FIXME output directory to put the dumps in WITH TRAILING SLASH
my $outputdir = "/scratch/dumps/";

# USER: FIXME generic arguments to dump
# This does NOT include the dump level, update flag (u) or the
# filename, all of which are arranged by the script automatically.
# WARNING: You should probably leave -L (live fs) and -a (autosize) in
# here or you risk corrupting your dumps!
# -h 0 honours the nodump fs flag for level 0 dumps and above.
my $dumpargs = "-L -a -h 0";

# USER: FIXME path to dump executable
my $dumppath = "/sbin/dump";

# USER: FIXME path to compression executable
my $zippath = "/usr/bin/bzip2";

# USER: FIXME path to decompress-to-stdout executable
my $zcatpath = "/usr/bin/bzcat";

# USER: FIXME path to restore
# (used only for checking; this script does not restore backups)
my $restpath = "/sbin/restore";


#dump file format is <hostname>-<mountpoint>-l<level>-yyyy-mm-dd.dump.bz2
#e.g. servar-root-l0-2006-03-03.dump.bz2
#e.g. servar-usr-l5-2006-03-08.dump.bz2

# init values used later
my @now = localtime(time);
my $timestamp = strftime "%F", @now; #YYYYMMDD for files
my $wday = $now[6];
my $mday = $now[3];
my $hostname = `hostname -s`;
chop $hostname;
my $level = 0;

#sanity checking day arguments, this should really never happen
die ("FATAL ERROR: Bad weekday!") if ($wday > 6 || $wday < 0);
die ("FATAL ERROR: Bad day of month!") if ($mday > 31 || $mday < 1);


#decide what level we need to do, based on the chosen algorithm
if (SCHEDULE eq "W0")
{
   die() if ($wday != 0); #only need to dump on sunday
}
elsif (SCHEDULE eq "W0D1")
{
   if ($wday == 0) {$level = 0;}
   else            {$level = 1;}
}
elsif (SCHEDULE eq "W0INC")
{
   $level = $wday;
}
elsif (SCHEDULE eq "7TOH")
{
   if    ($wday == 0) {$level = 0;}
   elsif ($wday == 1) {$level = 3;}
   elsif ($wday == 2) {$level = 2;}
   elsif ($wday == 3) {$level = 5;}
   elsif ($wday == 4) {$level = 4;}
   elsif ($wday == 5) {$level = 7;}
   elsif ($wday == 6) {$level = 6;}
   else {die("FATAL ERROR: Unknown weekday!");}
}
elsif (SCHEDULE eq "MWINC")
{
   if ($mday == 1) {$level = 0;}
   else            {$level = $wday + 1;}
}
elsif (SCHEDULE eq "MWTOH")
{
   if ($mday == 1) {$level = 0;}
   elsif ($wday == 0) {$level = 1;}
   elsif ($wday == 1) {$level = 3;}
   elsif ($wday == 2) {$level = 2;}
   elsif ($wday == 3) {$level = 5;}
   elsif ($wday == 4) {$level = 4;}
   elsif ($wday == 5) {$level = 7;}
   elsif ($wday == 6) {$level = 6;}
   else {die("FATAL ERROR: Unknown weekday!");}
}
else
{
   die("FATAL ERROR: Schedule " . SCHEDULE . " not implemented.\n");
}


my $dumpfile;
my $command;

# dump all
foreach my $fs (keys %fstodump)
{
   $dumpfile = "$outputdir$hostname-$fs-l$level-$timestamp.dump.bz2";

   #make sure it doesn't already exist
   if (-e $dumpfile)
   {
      print("\nWARNING: $dumpfile exists, skipping assuming already dumped.\n");
      next;
   }

   print("\nDumping $fs ($fstodump{$fs}) at level $level to file $dumpfile.\n");

   # NOTE the u appears straight after level
   $command = "$dumppath -".$level.
                 "u $dumpargs -f - $fstodump{$fs} | $zippath > $dumpfile";

   print("Executing $command\n\n");
   warn("ERROR: $command Failed!\n\n") if (system($command) != 0);
}

# check all
foreach my $fs (keys %fstodump)
{
   $dumpfile = "$outputdir$hostname-$fs-l$level-$timestamp.dump.bz2";
   print("\nChecking integrity of dump $dumpfile.\n");

   if (0 == $level)
   {
      # -r -N to extract and discard, -t to list
      $command = "$zcatpath $dumpfile | $restpath -r -N -f -";
      print("Executing $command\n\n");
   }
   else
   {
      # -t to list
      $command = "$zcatpath $dumpfile | $restpath -t -f - ";
      print("Executing $command\n\n");
   }

   warn("ERROR: $command Failed!\n\n") if (! `$command`); #discard listing
}

