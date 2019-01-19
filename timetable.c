/* $Id: timetable.c 103 2010-07-20 16:30:45Z dleigh $
   vim: tabstop=3 shiftwidth=3 expandtab:


Reads in a ~/.timetable with timetable information and tells you what
classes (or whatever) you need to go to next. The timetable display
starts at the entry which has the closest end time (i.e. a class on when
the program is invoked will be displayed).

Commandline:

   -m Monochrome (disables ansi colours)

   <n> Days forward to print timetable data
       (includes today; clamped 1-7; default 2)

   -r reverse sorting order (most recent first)

   -b Print a concise plot showing when you are busy;
      this disables all other options

   -B as -b but ignores days on which nothing occurs.

   -c toggle "codes" in the -b and B mode.
      If the first char of the description is '?!@$%^&*' the plot will
      use that character (default is #).

   -f <filename> use filename as .timetable file

   -e Invoke editor on the .timetable file;
      this disables all other options

   -p Prints out a timetable for each day on "standard" 66x80 pages
      (pipe through mpage -t -4 for a double sided weekly timetable);
      this disables all other options

   -P as -p but ignores days on which nothing occurs.

~/.timetable format:

   <weekday> <start time> <end time> <description>

   - weekday must be "mon", "tue" etc. It must be in lowercase.
   - Start and end times are in hh:mm format.
   - Descriptions can be multiple words and use any characters (apart from
     nulls, newlines etc).
   - Blank lines and comment lines (starting with #) will be ignored.

   Examples:

   tue 12:30 16:00 **** Work shift ****
   mon 15:30 17:30 (10.11.04) Micros Lecture
   # this is a comment
   wed 14:30 15:00 Project Group Meeting

   -----

Portions based on Emil Mikulic's todo.c: http://dmr.ath.cx/stuff/code/todo.c

   -----

 Copyright (c) 2004-2009 Dylan Leigh.


 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Modified versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

 4. The names of the authors or copyright holders must not be used to
    endorse or promote products derived from this software without prior
    written permission.


 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.

 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

    -----

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

/* these are all with black background (40) */
#define ANSI_RED     "\033[0;31m"
#define ANSI_GREEN   "\033[0;32m"
#define ANSI_YELLOW  "\033[0;33m"
#define ANSI_BLUE    "\033[0;34m"
#define ANSI_MAGENTA "\033[0;35m"
#define ANSI_CYAN    "\033[0;36m"
#define ANSI_WHITE   "\033[0;37m"
#define ANSI_NORMAL  "\033[0m"

#define DAYSECONDS 86400
#define BUFLEN 256

char * homeDir = NULL;
char * ttFile  = NULL;
#define TTFILENAME ".timetable"

char monochrome = 0;
char busycodes = 1;
char mode = 0;    //B, E, r as commandline
unsigned char days = 2; //1 = today, max 7
unsigned linenum = 0;
unsigned debugMode = 0;
#define MAX_DAYS 7

time_t now, limit, today;
unsigned thisWeekday;
unsigned thisYear;


/* Assuming people run the program at a random time this should be
 * efficient, as times are pushed forward after the now time */
struct _tt_entry
{
   time_t start;
   time_t end;
   char * desc;
   unsigned days;

   struct _tt_entry * closer;
   struct _tt_entry * further;
};

typedef struct _tt_entry TTEntry;
TTEntry * head;


static void *
xmalloc(const size_t s)
{
   void *tmp = malloc(s);
   if (!tmp)
   {
      fprintf(stderr, "Out of memory!\n");
      exit(EXIT_FAILURE);
   }
   return tmp;
}

/* not thread safe */
static char *
getline(FILE *f)
{
   static char line[BUFLEN];
   int i;
   char c;

   linenum++;
   for (i=0; i<BUFLEN-1 && (c=(char)fgetc(f)) != '\n' && !feof(f); i++)
      line[i] = c;
   line[i] = 0;

   if (debugMode)
      printf("\nRead line %d : %s\n", linenum, line);

   if (i == BUFLEN-1)
   {
      fprintf(stderr, "Line too long at %s:%d\n", ttFile, linenum);
      return NULL;
   }
   return line;
}

static TTEntry *
insert(TTEntry * node, TTEntry * newent)
{
   if (node == NULL)
   {
      if (debugMode)
         printf("Inserting %s\n", newent->desc);
      return newent;
   }

   if (debugMode)
      printf("Parsing %s\n", node->desc);

   if (newent->end > node->end)
      node->further = insert(node->further, newent);
   else if (newent->end < node->end)
      node->closer = insert(node->closer, newent);
   else
   {
      if (newent->start > node->start)
         node->further = insert(node->further, newent);
      else
         node->closer = insert(node->closer, newent);
   }

   return node;
}

static TTEntry *
add_TTEntry(int shour, int smin, int ehour, int emin, int weekday, char * desc)
{
   TTEntry * newent = NULL;
   int daysAway;
   time_t end;
   time_t start;
   
   //determine how many days this entry is from the future
   if (weekday < thisWeekday)
      daysAway = weekday - thisWeekday + 7;
   else
      daysAway = weekday - thisWeekday;

   if (debugMode)
      printf("shour:%d, smin:%d, ehour:%d, emin:%d, weekday:%d daysAway:%d\n",
              shour, smin, ehour, emin, weekday, daysAway);

   if (limit && daysAway > days)
   {
      if (debugMode)
         printf("daysAway > days (%d)\n", days);
      free(desc);
      return NULL;
   }

   //create end time
   end = today + (daysAway * DAYSECONDS) + (ehour * 3600) + (emin * 60);
   if (debugMode)
      printf("end: %d\n", end);

   if (limit && end < now)
   {
      free(desc);
      return NULL;
   }

   //create start time
   start = today + (daysAway * DAYSECONDS) + (shour * 3600) + (smin * 60);
   if (debugMode)
      printf("start: %d\n", start);

   if (start > end)
   {
      fprintf(stderr, "Start after end at %s:%d.\n", ttFile, linenum);
      free(desc);
      return NULL;
   }

   //create the TTEntry
   newent = xmalloc(sizeof(TTEntry));
   newent->start = start;
   newent->end = end;
   newent->desc = desc;
   newent->further = NULL;
   newent->closer = NULL;
   newent->days = daysAway;

   head = insert(head, newent);
   return newent;
}

static TTEntry *
parse_ttline(char * line)
{
   char * desc;
   unsigned left = 0, right = 0;
   int shour = 0, smin = 0, ehour = 0, emin = 0, weekday = 0;
   
   if (!strlen(line)) return NULL;  //blank line
   if (line[0] == '#') return NULL; //comment

   while (line[right] == ' ' && line[right])
      right++; // whitespace

   if (!line[right]) return NULL; //blank line
   if (line[right] == '#')
      return NULL; //comment


   //save the entire line here for the description
   desc = strdup(line+right);

   //weekday

   left = right;
   while (line[right] != ' ' && line[right])
       right++; // not whitespace
   if (!line[right])
   {
      fprintf(stderr, "Malformed line in %s:%d.\n", ttFile, linenum);
      return NULL;
   }

   line[right] = '\0';
   if (debugMode)
      printf("Line %d weekday: %s\n", linenum, line+left);
   //convert into struct tm integer
   if (strcmp("sun", line+left) == 0) weekday = 0; else
   if (strcmp("mon", line+left) == 0) weekday = 1; else
   if (strcmp("tue", line+left) == 0) weekday = 2; else
   if (strcmp("wed", line+left) == 0) weekday = 3; else
   if (strcmp("thu", line+left) == 0) weekday = 4; else
   if (strcmp("fri", line+left) == 0) weekday = 5; else
   if (strcmp("sat", line+left) == 0) weekday = 6; else
   {
      fprintf(stderr, "Unrecognised weekday in %s:%d.\n", ttFile, linenum);
      return NULL;
   }
   if (debugMode)
      printf("Line %d weekday: %d\n", linenum, weekday);


   //start hour

   left = ++right;
   while (line[right] == ' ' && line[right])
       right++; // whitespace
   if (!line[right])
   {
      fprintf(stderr, "Malformed line in %s:%d.\n", ttFile, linenum);
      return NULL;
   }

   left = right;
   while (line[right] != ':' && line[right])
       right++; // not seperator
   if (!line[right])
   {
      fprintf(stderr, "Malformed line in %s:%d.\n", ttFile, linenum);
      return NULL;
   }

   line[right] = '\0';
   if (debugMode)
      printf("Line %d start hour: %s\n", linenum, line+left);
   shour = atoi(line+left);


   //start minute

   left = ++right;
   while (line[right] != ' ' && line[right])
       right++; // not whitespace
   if (!line[right])
   {
      fprintf(stderr, "Malformed line in %s:%d.\n", ttFile, linenum);
      return NULL;
   }

   line[right] = '\0';
   if (debugMode)
      printf("Line %d start minute: %s\n", linenum, line+left);
   smin = atoi(line+left);


   //end hour

   left = ++right;
   while (line[right] == ' ' && line[right])
       right++; // whitespace
   if (!line[right])
   {
      fprintf(stderr, "Malformed line in %s:%d.\n", ttFile, linenum);
      return NULL;
   }

   left = right;
   while (line[right] != ':' && line[right])
       right++; // not seperator
   if (!line[right])
   {
      fprintf(stderr, "Malformed line in %s:%d.\n", ttFile, linenum);
      return NULL;
   }

   line[right] = '\0';
   if (debugMode)
      printf("Line %d end hour: %s\n", linenum, line+left);
   ehour = atoi(line+left);


   //end minute

   left = ++right;
   while (line[right] != ' ' && line[right])
       right++; // not whitespace
   if (!line[right])
   {
      fprintf(stderr, "Malformed line in %s:%d.\n", ttFile, linenum);
      return NULL;
   }

   line[right] = '\0';
   if (debugMode)
      printf("Line %d end minute: %s\n", linenum, line+left);
   emin = atoi(line+left);

   return add_TTEntry(shour, smin, ehour, emin, weekday, desc);
}

static void
read_ttfile()
{
   FILE *f = fopen(ttFile, "r");

   linenum = 0;

   if (!f)
   {
      fprintf(stderr, "Can't open %s\n", ttFile);
      exit(EXIT_FAILURE);
   }

   while (!feof(f))
   {
      char * tmp = getline(f);
      if (tmp)
         parse_ttline(tmp);   //discard - parse_ adds it
      else
         break;
   }
   fclose(f);
}

static void
printTree(TTEntry * node)
{
   if (node == NULL) return;

   if (node->closer) printTree(node->closer);

   if (monochrome)
      printf("%s\n", node->desc);
   else if (node->start < now)
      printf("%s%s%s\n", ANSI_RED, node->desc, ANSI_NORMAL);
   else if (node->days == 0)
      printf("%s%s%s\n", ANSI_YELLOW, node->desc, ANSI_NORMAL);
   else if (node->days == 1)
      printf("%s%s%s\n", ANSI_CYAN, node->desc, ANSI_NORMAL);
   else
      printf("%s%s%s\n", ANSI_GREEN, node->desc, ANSI_NORMAL);

   if (node->further) printTree(node->further);
}

static void
do_timetable()
{
   //Print raw time info for debugging
   if (debugMode)
   {
      printf("now:%d today:%d limit:%d year:%d weekday:%d\n",
              now, today, limit, thisYear, thisWeekday);
   }

   //read (and sort) the ttfile
   read_ttfile();

   //print the entries in order
   //only those lower than limit will have been added
   printTree(head);
}

static void
do_editor()
{
   char *cmd;
   char *editor = getenv("EDITOR");

   if (!editor || !strlen(editor))
   {
      printf("You need to define the EDITOR environment variable!\n");
      exit(EXIT_FAILURE);
   }

   cmd = (char*)xmalloc(strlen(editor) + strlen(ttFile) + 2);
   sprintf(cmd, "%s %s", editor, ttFile);
   system(cmd);
   free(cmd);
}

/// returns the decirption of what is on at that time, or NULL
/// Will return the FIRST entry found, does not check for clashes.
/// TODO: add clashes together somehow or warn on STDERR
static char *
check_time(time_t tm, TTEntry * ent)
{
   if (ent == NULL) return NULL;

   if (ent->start > tm)
      return check_time(tm, ent->closer);

   if (ent->end <= tm)
      return check_time(tm, ent->further);

   //if we reach here it's in this time
   return ent->desc;
}

static void
do_usage()
{
printf("Usage:\n\
   -m Monochrome (disables ansi colours)\n\
   <n> Days forward to print timetable data\n\
       (includes today; clamped 1-7; default 2)\n\
   -r reverse sorting order (most recent first)\n\
   -c toggle \'codes\' in the -b and B mode.\n\
      If the first char of the description is '?!@$\%%^&*' the plot will\n\
      use that character (default is #).\n\
   -b Print a concise plot showing when you are busy;\n\
      this disables all other options\n\
   -B As -b but ignores days on which no events occur.\n\
   -f <filename>     Use <filename> as .timetable file\n\
   -e Invoke editor on the .timetable file;\n\
      this disables all other options\n\
   -p Prints out a timetable for each day on \"standard\" 66x80 pages\n\
      (pipe through mpage -t -4 for a double sided weekly timetable);\n\
      this disables all other options\n\
   -P As -p but ignores days on which no events occur.\n\
      this disables all other options\n");
   exit(EXIT_FAILURE);
}

//returns TRUE if there is ANYTHING between the given times
static int
busy_time(TTEntry * ent, time_t start, time_t end)
{
   if (ent == NULL) return 0;

   if (ent->end < start)
      return busy_time(ent->further, start, end);

   if (ent->start > end)
      return busy_time(ent->closer, start, end);

   //this entry is between start and end, somewhere
   return 1;
}

//returns TRUE if there is ANYTHING on on the day
static int
busy_day(int day)
{
   time_t start, end;
   int daysAway;

   //determine how many days the day is from the future
   if (day < thisWeekday)
      daysAway = day - thisWeekday + 7;
   else
      daysAway = day - thisWeekday;

   start = today + (DAYSECONDS * daysAway);
   end = today + (DAYSECONDS * (daysAway + 1));

   return busy_time(head, start, end);
}

//prints one day on one standard 66 line by 80 char page
static void
print_day(int day)
{
   time_t tm, start, end;
   int daysAway;

   //determine how many days the day is from the future
   if (day < thisWeekday)
      daysAway = day - thisWeekday + 7;
   else
      daysAway = day - thisWeekday;

   start = today + (DAYSECONDS * daysAway);
   end = today + (DAYSECONDS * (daysAway + 1));

   switch(day)
   {
      case 0: printf("\n               SUNDAY\n\n");
              break;
      case 1: printf("\n               MONDAY\n\n");
              break;
      case 2: printf("\n               TUESDAY\n\n");
              break;
      case 3: printf("\n               WEDNESDAY\n\n");
              break;
      case 4: printf("\n               THURSDAY\n\n");
              break;
      case 5: printf("\n               FRIDAY\n\n");
              break;
      case 6: printf("\n               SATURDAY\n\n");
              break;
      case 7: printf("\n               SUNDAY\n\n");
              break;
      default: fprintf(stderr, "Invalid day %d!\n", day);
               exit(EXIT_FAILURE);
               break;
   }

   for(tm=start; tm<end; tm+=1800)
   {
      char * desc = check_time(tm, head);
      if (desc == NULL)
         printf("-\n");
      else
         printf("%s\n", desc+4);
   }

   printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); //pagination
}

//prints all days - use mpage -t 4 do put one week on one 2 sided page
static void
do_printable()
{
   read_ttfile();

   if (mode == 'P')
   {
      if (busy_day(1)) print_day(1);
      if (busy_day(2)) print_day(2);
      if (busy_day(3)) print_day(3);
      if (busy_day(4)) print_day(4);
      if (busy_day(5)) print_day(5);
      if (busy_day(6)) print_day(6);
      if (busy_day(0)) print_day(0);
      //printf(":P\n");
   }
   else
   {
      print_day(1);
      print_day(2);
      print_day(3);
      print_day(4);
      print_day(5);
      print_day(6);
      print_day(0);
   }
}

static void
busy_line(int day)
{
   time_t tm, start, end;
   int daysAway;

   //determine how many days the day is from the future
   if (day < thisWeekday)
      daysAway = day - thisWeekday + 7;
   else
      daysAway = day - thisWeekday;

   start = today + (DAYSECONDS * daysAway);
   end = today + (DAYSECONDS * (daysAway + 1));

   //trim from 0700-2300
   start += 3600 * 7;
   end -= 3600;

   switch (day)
   {
      case 1: printf("mon |"); break;
      case 2: printf("tue |"); break;
      case 3: printf("wed |"); break;
      case 4: printf("thu |"); break;
      case 5: printf("fri |"); break;
      case 6: printf("sat |"); break;
      case 0: printf("sun |"); break;
      default: fprintf(stderr, "Invalid day %d!\n", day);
               exit(EXIT_FAILURE);
               break;
   }

   for(tm=start; tm<end; tm+=1800)
   {
      char * desc = check_time(tm, head);
      if (NULL == desc)
         printf(" |");
      else
      {
         if (0 == busycodes)
            printf("#|");
         else
         {
            switch (desc[16])
            {
               case '?':
               case '!':
               case '@':
               case '$':
               case '%':
               case '^':
               case '&':
               case '*':
                  printf("%c|", desc[16]);
                  break;

               default:
                  printf("#|");
                  break;
            }
         }
      }
   }
   printf("\n");
}

static void
do_busy()
{
   read_ttfile();

   //print header lines
   printf(
"    |7  |8  |9  |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |21 |22 |\n");

   if (mode == 'B')
   {
      if (busy_day(1)) busy_line(1);
      if (busy_day(2)) busy_line(2);
      if (busy_day(3)) busy_line(3);
      if (busy_day(4)) busy_line(4);
      if (busy_day(5)) busy_line(5);
      if (busy_day(6)) busy_line(6);
      if (busy_day(0)) busy_line(0);
   }
   else
   {
      busy_line(1);
      busy_line(2);
      busy_line(3);
      busy_line(4);
      busy_line(5);
      busy_line(6);
      busy_line(0);
   }
}

int main(int argc, char **argv)
{
   int i;
   struct tm * tmptm;

   homeDir = getenv("HOME");

   //set now, today, thisWeekday, thisYear - limit set later
   now = time(NULL);
   tmptm = localtime(&now);
   thisWeekday = tmptm->tm_wday;
   thisYear = tmptm->tm_year; // + 1900
   tmptm->tm_sec = 0;
   tmptm->tm_min = 0;
   tmptm->tm_hour = 0;
   today = mktime(tmptm);

   // check args
   for (i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "-f") == 0)
      {
         i++;
         if (i >= argc) do_usage();
         ttFile = argv[i];
      } else
      if (strcmp(argv[i], "-m") == 0) monochrome = 1; else
      if (strcmp(argv[i], "-c") == 0) busycodes = !busycodes; else
      if (strcmp(argv[i], "-b") == 0) mode = 'b'; else
      if (strcmp(argv[i], "-B") == 0) mode = 'B'; else
      if (strcmp(argv[i], "-e") == 0) mode = 'e'; else
      if (strcmp(argv[i], "-r") == 0) mode = 'r'; else
      if (strcmp(argv[i], "-p") == 0) mode = 'p'; else
      if (strcmp(argv[i], "-P") == 0) mode = 'P'; else
      if (strcmp(argv[i], "-DEBUG") == 0) debugMode = 1; else
      {
         days = (int)strtol(argv[i], NULL, 10);
         if (!days)
            do_usage();
      }
   }

   //timetable file
   if (ttFile == NULL)
   {
      ttFile = (char*) xmalloc(strlen(homeDir) + strlen(TTFILENAME) + 2);
      sprintf(ttFile, "%s/%s", homeDir, TTFILENAME);
   }

   if (days > MAX_DAYS) days = MAX_DAYS;

   switch(mode)
   {
      case 'B':
      case 'b': limit = 0; //disable
                do_busy();
                break;

      case 'P':
      case 'p': limit = 0; //disable
                do_printable();
                break;

      case 'e': do_editor();
                break;

      default : limit = today + (days * DAYSECONDS); //60s * 60m * 24h
                do_timetable();
                break;
   }

   return EXIT_SUCCESS;
}
