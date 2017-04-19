#!/usr/local/bin/perl
# $Id: dice.pl 92 2009-11-20 01:36:27Z dleigh $
# vim: set expandtab cindent tabstop=3 shiftwidth=3 textwidth=78:

#  dice.pl - Dice roller with Silhouette, Alternity and Shadowrun support.

# Usage: !dice <dice to roll, default 1>d<sides per die, default 6>
#        e.g. "dice 2d" "dice 2d20"

#        Use "-" to read arguments from stdin, until EOF, "exit" or "quit"

#        To make Silhouette dice rolls use "s" for the sides (e.g. 3ds) or the
#        form "s3" (for skill 3). For unskilled tests use "0ds" or "s0" (i.e.
#        skill 0).

#        To make Shadowrun rolls use "3t5" where 3 is the dice to roll and 5
#        is target number. Rule of six is handled (rule of one is not,
#        exactly, but you are not allowed target numbers of 1 or 0).

#        Use "a4" for an Alternity roll, where the number indicates the
#        dituation die (in this case a d4). Prepend a minus for a minus die
#        (e.g. a-4). Put multipliers before the a (e.g. "3a20" for a maximum
#        penalty situation).


# Output will be the value of each die rolled, followed by the sum (or the
# final value for silhouette/alternity rolls, or number of successes for
# shadowrun rolls).

#
# -----
#
# Copyright (c) 2004-2005 Dylan Leigh.
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
# 4. The names of the authors or copyright holders must not be used to endorse
#    or promote products derived from this software without prior written
#    permission.
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

#limits
use constant MINSIDES => 2;
use constant MAXSIDES => 65536;
use constant DEFSIDES => 6;
use constant MAXROLLS => 32;

use strict;
use warnings;

sub makeRolls
{
   #usage makerolls <numdice> <numsides>
   #returns array of values

   my $i;
   my @ret = ();
   for($i = 0; $i < $_[0]; $i++)
   {
      push(@ret, int(rand($_[1]))+1);
   }

   return @ret;
}

sub worker
{
   my $cmd = $_[0];     #main argument
   my $value = 0;       #final input value
   my $i = 0;           #spare iterator
   my @rolls;           #holds actual dice rolls
   
   if ($cmd =~ /^(\d*)a(-?)(\d+)$/)
   {
      #alternity
      die "ERROR: Too many sides!\n" if ($3 > MAXSIDES);

      my $cont = int(rand(20))+1;
      print "Control die $cont, ";

      if ($3 == 0)
      {
         print "No situation die\n";
         @rolls = ();
      }
      elsif ($1 eq "")
      {
         @rolls = (int(rand($3))+1);
         print "Situation die @rolls\n";
      }
      else
      {
         die "ERROR: Too many dice!\n" if ($1 > MAXROLLS);
         @rolls = makeRolls($1, $3);
         print "Situation die @rolls\n";
      }

      if ($2 eq "-")
      {
         $value = $cont;
         foreach (@rolls)
         {
            $value -= $_;
         }
      }
      else
      {
         $value = $cont;
         foreach (@rolls)
         {
            $value += $_;
         }
      }
      print "Final result is $value\n";
   }
   elsif ($cmd =~ /^(\d+)ds$/ || $cmd =~ /^s(\d+)$/)
   {
      #silhouette
      die "ERROR: Too many dice!\n" if ($1 > MAXROLLS);

      if ($1 == 0)
      {
         #special skill level 0 roll
         #lower of 2, no rule of six
         $value = 6;
         @rolls = makeRolls(2, 6);
         foreach (@rolls)
         {
            $value = $_ if ($value > $_)
         }
      }
      else
      {
         @rolls = makeRolls($1, 6);
         foreach (@rolls)
         {
            if ($value >= 6 && $_ == 6)
            {
               $value++;
            }
            $value = $_ if ($value < $_)
         }
      }

      print "Level $1 roll... @rolls\n";
      if ($value < 2)
      {
         print "Fumble!\n";
      }
      else
      {
         print "Final result is $value\n";
      }
   }
   elsif ($cmd =~ /^(\d+)t(\d+)$/)
   {
      #shadowrun
      die "ERROR: No dice to roll?\n" if ($1 < 1);
      die "ERROR: Too many dice!\n" if ($1 > MAXROLLS);
      die "ERROR: Illegal target number\n" if ($2 < 2);

      @rolls = makeRolls($1, 6);
      print "Rolling $1... \n";

      foreach (@rolls)
      {
         print "$_ ";

         #rule of six
         $i = $_;
         print "(" if ($i == 6);
         while($_ == 6)
         {
            $_ = int(rand(6))+1;
            $i += $_;
            print "+$_";
         }
         print ") " if ($i > 6);
         
         $value++ if ($i >= $2);
      }

      print "\n $value successes. \n";
   }
   elsif ($cmd =~ /^(\d*)d(\d*)$/)
   {
      #normal dice
      my $dice = $1;
      my $sides = $2;

      #defaults
      $dice = 1 if $dice eq "";
      $sides = DEFSIDES if $sides eq "";

      die "ERROR: No dice to roll?" if ($dice < 1);
      die "ERROR: Too many dice!\n" if ($dice > MAXROLLS);
      die "ERROR: Too many sides!\n" if ($sides > MAXSIDES);
      die "ERROR: Can't find my $2 sided die\n" if ($sides < MINSIDES);

      print "Rolling $dice d$sides... ";

      @rolls = makeRolls($dice, $sides);
      foreach (@rolls)
      {
         print "$_ ";
         $value += $_;
      }

      print "\n";
      if ($dice != 1)
      {
         print "Total sum is $value\n";
      }
   }
   else  #failure
   {
      warn "ERROR: Invalid argument.\n";
   }
}

#main()

foreach (@ARGV)
{
   if ($_ eq "-")
   {
      #read arguments from stdin
      my $line;
      while($line = <STDIN>)
      {
         chomp($line);
         last if ($line eq "exit" || $line eq "quit");
         worker($line);
      }
   }
   else
   {
      worker($_);
   }
}

