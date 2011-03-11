
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include "utils.h"
#include "node.h"

char* progname = NULL;

void
err(char* prompt, ...)
{
  va_list ap;
  va_start(ap,prompt);

   fprintf(stderr, "%s: Error: ", progname);
   vfprintf(stderr, prompt, ap);
   fprintf(stderr, "\n");
   exit(-1);
}

int
number_cpus(void)
{
  return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

int
random_int(int max)
{
  return rand() % max;
}
