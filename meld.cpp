#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/stat.h>
#include <iostream>

#include "config.h"

#include "process/exec.hpp"
#include "runtime/list.hpp"
#include "utils/utils.hpp"

using namespace utils;
using namespace process;

static void
help(void)
{
  fprintf(stderr, "meld: execute meld program\n");
  fprintf(stderr, "\t-f <name>:\tmeld program\n");
  fprintf(stderr, "\t-t <threads>:\tnumber of threads\n");

  exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
  bool has_program = false;
  bool has_threads = false;
  char *program = NULL;
	char *progname;
	size_t num_threads = 0;

  progname = *argv++;
  --argc;

  while (argc > 0 && (argv[0][0] == '-')) {
    switch(argv[0][1]) {
    case 'f': {
        if (has_program || argc < 2)
          help();

        program = argv[1];

        has_program = true;
        argc--; argv++;
      }
      break;
    case 't': {
        if (has_threads || argc < 2)
          help();

        num_threads = (size_t)atoi(argv[1]);

        has_threads = true;
        argc--; argv++;
      }
      break;
    default:
      help();
    }

    /* advance */
    argc--; argv++;
  }

  if (program == NULL)
    help();

  if (!has_threads)
    num_threads = number_cpus();
    
 machine mac(program, num_threads);
 
 mac.start();
 
  /* not reached */
  return EXIT_SUCCESS;
}
