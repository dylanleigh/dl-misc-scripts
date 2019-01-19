/* $Id: randompw.c 83 2009-07-14 14:07:52Z dleigh $
 *
 * Generate a 8 character password randomly from alphanumeric
 * characters, using arc4random() as the PRNG. arc4random() should be
 * crypographically random, certainly good enough for this purpose.
 *
 * TODO: first character alpha-only (required by some systems)
 *       option to do alphanum + something
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PASSLEN 8

#define NUMONLY "1234567890" // Could do it numerically in this case...
#define ALPHAONLY \
   "qwertyuiopQWERYTUIOPasdfghjklASDFGHJKLzxcvbnmZXCVBNM"
#define ALPHANUM \
   "qwertyuiopQWERYTUIOPasdfghjklASDFGHJKLzxcvbnmZXCVBNM1234567890"

   /* There are too many special cases for the following to be really
    * practical, if it was used as-is. A better approach would be to
    * just use a certain subset of ASCII with an option to exclude a
    * specified set of characters */
#define ALPHANUMSPECIAL \
   "qwertyuiopQWERYTUIOPasdfghjklASDFGHJKLzxcvbnmZXCVBNM1234567890_-+="

void do_usage()
{
   // TODO: print usage message
}

int main(int argc, char ** argv)
{
   int i = 0;                       // iterator
   int firstAlpha = 0;              // First character alphabetic
   char * validChars = ALPHANUM;    // Character set to use
   
   //parse commandline
   for (i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--alpha") == 0) validChars = ALPHAONLY;
      if (strcmp(argv[i], "-n") == 0) validChars = NUMONLY;
      if (strcmp(argv[i], "--num") == 0) validChars = NUMONLY;
      if (strcmp(argv[i], "--an") == 0) validChars = ALPHANUM;
      if (strcmp(argv[i], "--all") == 0) validChars = ALPHANUMSPECIAL;

      if (strcmp(argv[i], "--chars=") == 0)  // use characters from CLI
      {
         if (++i < argc)   // next argument exists?
            validChars = argv[i];
         else
         {
            do_usage();
            return EXIT_FAILURE;
         }
      }

      if (strcmp(argv[i], "--firstalpha") == 0) firstAlpha = 1;
      // TODO: firstalpha not implemented
   }

   validCharSize = strlen(validChars) - 1;
   if (validCharSize < 2)
   {
      do_usage();
      return EXIT_FAILURE;
   }

   // generate the password
   for (i = 0; i < PASSLEN; i++)
      putc(validChars[arc4random() % validCharSize]);

   return EXIT_SUCCESS;
}
