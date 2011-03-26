#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/stat.h>

#include "config.h"

#include "node.h"
#include "utils.h"
#include "thread.h"
#include "vm.h"
#include "list.h"
#include "barrier.h"
#include "hash.h"
#include "partition.h"
#include "list_runtime.h"

static NodeID nodes_per_thread = 0;

typedef void *(*NodeProgram)(Node* n);
typedef void *(*GenericThreadProgram)(void*);

static void
help(void)
{
  fprintf(stderr, "meld: execute meld program\n");
  fprintf(stderr, "\t-f <name>:\tmeld program\n");
  fprintf(stderr, "\t-t <threads>:\tnumber of threads\n");

  exit(EXIT_SUCCESS);
}


static void
config_read(const char* file)
{
	(void)file;

	NodeID node1 = 1;
	NodeID node2 = 2;
	NodeID node3 = 3;

	Node* n1 = nodes_ensure(node1);
	Node* n2 = nodes_ensure(node2);
	Node* n3 = nodes_ensure(node3);

	node_add_neighbor(n1, n2);
	node_add_neighbor(n2, n3);
	node_add_neighbor(n3, n1);
}

static void*
thread_launch(void *data)
{
	int thid = (int)(Register)data;
	Thread *th = (Thread *)meld_malloc(sizeof(Thread));

	threads[thid] = th;
	th->id = thid;

	thread_init(th);

  printf("Launched thread %d\n", th->id);

  int start = th->id * nodes_per_thread;
  int nodes;
  
  if(th->id == NUM_THREADS - 1) /* last thread */
    nodes = nodes_total() - start;
  else
    nodes = nodes_per_thread;
    
  printf("Copying %d nodes to thread %d\n", nodes, th->id);

	partition_do(HASH_NODES, th, start, nodes);

	partition_finish(th);

  thread_run(th);

  pthread_exit(NULL);
}

static void
threads_setup(void)
{
  /* in the worst case, there is one thread per node */
  if (nodes_total() < NUM_THREADS) {
    fprintf(stderr, "There are more threads than nodes.\n");
    exit(EXIT_FAILURE);
  }

  threads = (Thread**)meld_malloc(sizeof(Thread*) * NUM_THREADS);
  if (threads == NULL)
    err("Failed to create %d thread objects", NUM_THREADS);
  
  printf("Launching %d threads\n", NUM_THREADS);

  /* distribute nodes over threads */
  nodes_per_thread = (nodes_total() + NUM_THREADS/2) / NUM_THREADS;

  printf("Nodes Per Thread: " NODE_FORMAT "\n", nodes_per_thread);

  partition_init(NUM_THREADS);
}

static void
threads_launch(void)
{
  int i;
  int status = 0;
  pthread_t tid;
  pthread_attr_t attr;
  
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  for(i = 0; i < NUM_THREADS; ++i) {
    if (NUM_THREADS == i + 1)
      thread_launch((void*)(Register)i); /* the main thread */
    else
      status = pthread_create(&tid, &attr,
        (GenericThreadProgram)thread_launch, (void*)(Register)i);

    if (status != 0)
      err("Failed to create thread no %d: %id, %d\n", i + 1, status);
  }
}

static bool
program_read(const char *filename)
{
  FILE *fp = fopen(filename, "rb");
  
  if(fp == NULL)
    return 0;
    
  struct stat buf;
  if(-1 == fstat(fileno(fp), &buf)) {
    fclose(fp);
    return 0;
  }
  
  meld_prog = (unsigned char*)malloc(buf.st_size);
  if(meld_prog == NULL) {
    fclose(fp);
    return 0;
  }
  
  if(buf.st_size != fread(meld_prog, sizeof(byte), buf.st_size, fp)) {
    free(meld_prog);
    fclose(fp);
    return 0;
  }
  
  fclose(fp);
  
  return 1;
}

int
main(int argc, char **argv)
{
  bool has_program = false;
  bool has_threads = false;
  char *program = NULL;

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

        NUM_THREADS = atoi(argv[1]);

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
    NUM_THREADS = number_cpus();
    
  if(!program_read(program))
    help();
    
  if (NUM_THREADS < 1)
    NUM_THREADS = 1;

  vm_init();
    
  /* we now know the number of threads in the system */
  vm_threads_init();
  
	/* read the graph from the file and initiate nodes */
  nodes_init();
	config_read(NULL);

  threads_setup();
  threads_launch();

  /* not reached */
  return EXIT_SUCCESS;
}
