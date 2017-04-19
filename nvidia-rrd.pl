#!/usr/bin/env perl
#
# Stores the temperature of a nvidia GPU in a RRD file. If passed any
# commandline arguments, it will also create a graph. The file
# locations are hardcoded due to extreme laziness today.
#
#           -- dleigh 2012/01/07
#
#  rrdtool create /scratch/dleigh/gpu-temp.rrd -s 20
#  DS:temp:GAUGE:300:10:150 RRA:AVERAGE:0.5:1:86400
#  RRA:AVERAGE:0.5:3:172800 RRA:AVERAGE:0.5:15:8928

use strict;
use warnings;
use RRDs;

my $RRDB = '/scratch/dleigh/gpu-temp.rrd';
my $cmd = "nvidia-smi -q -d temperature";
my $outdir = "/tmp/";

# get data from nvidia-smi

my $data;
open PIPE, "$cmd |" or die $!;
while (<PIPE>)
{
   # find Gpu temperature line
   if ($_ =~ /Gpu *: ?(\d{2,3}) ?C/)
   {
      $data = $1;
      last;
   }
}
close PIPE;

# update the RRDB

RRDs::update ("$RRDB", "N:$data");
my $err = RRDs::error();
die ("FATAL: $err\n") if $err;

# subroutine to draw each graph

sub drawgraph {
   my $fname = shift or die "sub missing argument";
   my $secs = shift or die "sub missing argument";

   # create graphs
   RRDs::graph("$outdir/$fname",
            "-s -$secs",
            "DEF:temp=$RRDB:temp:AVERAGE",
            "AREA:temp#FF3366");
   $err = RRDs::error();
   warn("ERROR: $err\n") if $err;
}

if (shift)
{
   drawgraph("gpu-temp-15min.png",900);
   drawgraph("gpu-temp-90min.png",5400);
   drawgraph("gpu-temp-8hours.png",28800);
   drawgraph("gpu-temp-2day.png",172800);
   drawgraph("gpu-temp-2week.png",1209600);
   drawgraph("gpu-temp-2month.png",5356800);
}
