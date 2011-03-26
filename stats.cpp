
#include "core.h"
#include "stats.h"
#ifdef MACOSX
#include <mach/mach_time.h>
#endif
#include <time.h>

static stats_data stats;
#ifdef MACOSX
static uint64_t time_start;
#else
static timespec time_start;
#endif

static inline bool
thread_alive(void)
{
	return thread_self() != NULL;
}

static inline int
thread_id(void)
{
	return thread_self()->id;
}

void
stats_init(void)
{
	stats.proved_regular = 0;
	stats.retracted_regular = 0;
	stats.proved_linear = 0;
	stats.retracted_linear = 0;
	stats.consumed_linear = 0;
  stats.tuples_sent = 0;
  stats.tuples_sent_itself = 0;
  stats.tuples_allocated = 0;
  stats.lists_allocated = 0;
  stats.sets_allocated = 0;
	stats.run_gc = 0;
	stats.time_gc = 0.0;
	stats.lists_collected = 0;
  stats.sets_collected = 0;
	stats.time_exec = 0.0;
	printf("NUM THREADS %d\n", NUM_THREADS);
  
	stats.proved_regular_by_type = (big_counter *)meld_calloc(NUM_TYPES, sizeof(big_counter));
	stats.proved_regular_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
	stats.retracted_regular_by_type = (big_counter *)meld_calloc(NUM_TYPES, sizeof(big_counter));
	stats.retracted_regular_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
	stats.proved_linear_by_type = (big_counter *)meld_calloc(NUM_TYPES, sizeof(big_counter));
	stats.proved_linear_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
	stats.retracted_linear_by_type = (big_counter *)meld_calloc(NUM_TYPES, sizeof(big_counter));
	stats.retracted_linear_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
	stats.consumed_linear_by_type = (big_counter *)meld_calloc(NUM_TYPES, sizeof(big_counter));
	stats.consumed_linear_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
  stats.tuples_sent_by_type = (big_counter *)meld_calloc(NUM_TYPES, sizeof(big_counter));
	stats.tuples_sent_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
  stats.tuples_sent_itself_by_type = (big_counter *)meld_calloc(NUM_TYPES, sizeof(big_counter));
	stats.tuples_sent_itself_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
  stats.tuples_allocated_by_type = (big_counter *)meld_calloc(NUM_TYPES, sizeof(big_counter));
	stats.tuples_allocated_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
  stats.lists_allocated_by_type = (big_counter *)meld_calloc(LIST_NUM_TYPES, sizeof(big_counter));
	stats.lists_allocated_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));

  stats.sets_allocated_by_type = (big_counter *)meld_calloc(SET_NUM_TYPES, sizeof(big_counter));
	stats.sets_allocated_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
	stats.lists_collected_by_type = (big_counter *)meld_calloc(LIST_NUM_TYPES, sizeof(big_counter));
	stats.lists_collected_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));
  stats.sets_collected_by_type = (big_counter *)meld_calloc(SET_NUM_TYPES, sizeof(big_counter));
	stats.sets_collected_by_thread = (big_counter *)meld_calloc(NUM_THREADS, sizeof(big_counter));

	pthread_mutex_init(&stats.proved_regular_mutex, NULL);
	pthread_mutex_init(&stats.retracted_regular_mutex, NULL);
	pthread_mutex_init(&stats.consumed_linear_mutex, NULL);
	pthread_mutex_init(&stats.proved_linear_mutex, NULL);
	pthread_mutex_init(&stats.retracted_linear_mutex, NULL);
  pthread_mutex_init(&stats.tuples_sent_mutex, NULL);
  pthread_mutex_init(&stats.tuples_sent_itself_mutex, NULL);
  pthread_mutex_init(&stats.tuples_allocated_mutex, NULL);
  pthread_mutex_init(&stats.lists_allocated_mutex, NULL);
  pthread_mutex_init(&stats.sets_allocated_mutex, NULL);
	pthread_mutex_init(&stats.lists_collected_mutex, NULL);
  pthread_mutex_init(&stats.sets_collected_mutex, NULL);
}

void
stats_proved_tuple(tuple_type type, ref_count count)
{
	if(TYPE_IS_REGULAR(type))
		stats_proved_regular(type, count);
	else
		stats_proved_linear(type, count);
}

void
stats_retracted_tuple(tuple_type type, ref_count count)
{
	if(TYPE_IS_REGULAR(type))
		stats_retracted_regular(type, count);
	else
		stats_retracted_linear(type, count);
}

void
stats_proved_regular(tuple_type type, ref_count count)
{
	pthread_mutex_lock(&stats.proved_regular_mutex);

	stats.proved_regular += (big_counter)count;
	stats.proved_regular_by_type[type] += (big_counter)count;
	stats.proved_regular_by_thread[thread_id()] += (big_counter)count;

	pthread_mutex_unlock(&stats.proved_regular_mutex);
}

void
stats_retracted_regular(tuple_type type, ref_count count)
{
	pthread_mutex_lock(&stats.retracted_regular_mutex);

	stats.retracted_regular += (big_counter)count;
	stats.retracted_regular_by_type[type] += (big_counter)count;
	stats.retracted_regular_by_thread[thread_id()] += (big_counter)count;

	pthread_mutex_unlock(&stats.retracted_regular_mutex);
}

void
stats_consumed_linear(tuple_type type)
{
	pthread_mutex_lock(&stats.consumed_linear_mutex);

	stats.consumed_linear++;
	stats.consumed_linear_by_type[type]++;
	stats.consumed_linear_by_thread[thread_id()]++;

	pthread_mutex_unlock(&stats.consumed_linear_mutex);
}

void
stats_proved_linear(tuple_type type, ref_count count)
{
	pthread_mutex_lock(&stats.proved_linear_mutex);

	stats.proved_linear += (big_counter)count;
	stats.proved_linear_by_type[type] += (big_counter)count;
	stats.proved_linear_by_thread[thread_id()] += (big_counter)count;

	pthread_mutex_unlock(&stats.proved_linear_mutex);
}

void
stats_retracted_linear(tuple_type type, ref_count count)
{
	pthread_mutex_lock(&stats.retracted_linear_mutex);

	stats.retracted_linear += (big_counter)count;
	stats.retracted_linear_by_type[type] += (big_counter)count;
	stats.retracted_linear_by_thread[thread_id()] += (big_counter)count;

	pthread_mutex_unlock(&stats.retracted_linear_mutex);
}

void
stats_tuples_sent(tuple_type type)
{
  pthread_mutex_lock(&stats.tuples_sent_mutex);
  
  stats.tuples_sent++;
  stats.tuples_sent_by_type[type]++;
	stats.tuples_sent_by_thread[thread_id()]++;
  
  pthread_mutex_unlock(&stats.tuples_sent_mutex);
}

void
stats_tuples_sent_itself(tuple_type type)
{
  pthread_mutex_lock(&stats.tuples_sent_itself_mutex);
  
  stats.tuples_sent_itself++;
  stats.tuples_sent_itself_by_type[type]++;
	stats.tuples_sent_itself_by_thread[thread_id()]++;
  
  pthread_mutex_unlock(&stats.tuples_sent_itself_mutex);
}

void
stats_tuples_allocated(tuple_type type)
{
  pthread_mutex_lock(&stats.tuples_allocated_mutex);
  
  stats.tuples_allocated++;
  stats.tuples_allocated_by_type[type]++;
	if(thread_alive())
		stats.tuples_allocated_by_thread[thread_id()]++;
  
  pthread_mutex_unlock(&stats.tuples_allocated_mutex);
}

void
stats_lists_allocated(stat_list_type type)
{
  pthread_mutex_lock(&stats.lists_allocated_mutex);
  
  stats.lists_allocated++;
  stats.lists_allocated_by_type[type]++;
	if(thread_alive())
		stats.lists_allocated_by_thread[thread_id()]++;
  
  pthread_mutex_unlock(&stats.lists_allocated_mutex);
}

void
stats_sets_allocated(stat_set_type type)
{
  pthread_mutex_lock(&stats.sets_allocated_mutex);
  
  stats.sets_allocated++;
  stats.sets_allocated_by_type[type]++;
	stats.sets_allocated_by_thread[thread_id()]++;
  
  pthread_mutex_unlock(&stats.sets_allocated_mutex);
}

void
stats_run_gc(void)
{
	stats.run_gc++;
}

void
stats_lists_collected(stat_list_type type)
{
	pthread_mutex_lock(&stats.lists_collected_mutex);

	stats.lists_collected++;
	stats.lists_collected_by_type[type]++;

	pthread_mutex_unlock(&stats.lists_collected_mutex);
}

void
stats_sets_collected(stat_set_type type)
{
  pthread_mutex_lock(&stats.sets_collected_mutex);
  
  stats.sets_collected++;
  stats.sets_collected_by_type[type]++;
	stats.sets_collected_by_thread[thread_id()]++;
  
  pthread_mutex_unlock(&stats.sets_collected_mutex);
}

void
stats_time_gc(double elapsed)
{
	stats.time_gc += elapsed;
}

void
stats_start_exec(void)
{
#ifdef MACOSX
	time_start = mach_absolute_time();
#else
	clock_gettime(CLOCK_REALTIME, &time_start);
#endif
}

void
stats_end_exec(void)
{
#ifdef MACOSX
	uint64_t time_end = mach_absolute_time();

	uint64_t diff = time_end - time_start;
	static mach_timebase_info_data_t info = {0, 0};

	if(info.denom == 0)
		mach_timebase_info(&info);

	uint64_t elapsednano = diff * (info.numer / info.denom);
	struct timespec tp;

	tp.tv_sec = elapsednano * 1e-9;
	tp.tv_nsec = elapsednano - (tp.tv_sec * 1e9);

	double mili = tp.tv_nsec / (1000 * 1000);

	stats.time_exec = (double)(tp.tv_sec*1000) + mili;
#else
	clock_t time_end = clock();

	stats.time_exec = ((double) (time_end - time_start)) / (CLOCKS_PER_SEC / 1000);
#endif
}

void
stats_print(FILE *out)
{
	tuple_type i;

  fprintf(out, "\n");
  fprintf(out, "***************************************\n");
	fprintf(out, "*             STATISTICS:             *\n");
  fprintf(out, "***************************************\n\n");

#define SHOW_THREADS(total, vec)												\
	if(total > 0) {																				\
		fprintf(out, "    >>> Threads:\n");									\
		for(i = 0; i < NUM_THREADS; ++i) 										\
			fprintf(out, "    %d: %ld\n", i + 1, vec);				\
	}

#define SHOW_STAT(title, total, vec, threads)           \
  fprintf(out, "  " title ": %ld\n", total);            \
  for(i = 0; i < NUM_TYPES; ++i)                        \
    if((vec) > 0)                                       \
      fprintf(out, "    %s: %ld\n", TYPE_NAME(i), vec); \
	SHOW_THREADS(total, threads);													\
  fprintf(out, "\n")

  SHOW_STAT("Proved tuples", stats.proved_regular + stats.proved_linear,
      stats.proved_regular_by_type[i] + stats.proved_linear_by_type[i],
			stats.proved_regular_by_thread[i] + stats.proved_linear_by_thread[i]);

  SHOW_STAT("Retracted tuples", stats.retracted_regular + stats.retracted_linear,
	    stats.retracted_regular_by_type[i] + stats.retracted_linear_by_type[i],
			stats.retracted_regular_by_thread[i] + stats.retracted_linear_by_thread[i]);

  SHOW_STAT("Proved regular tuples", stats.proved_regular,
			stats.proved_regular_by_type[i],
			stats.proved_regular_by_thread[i]);

  SHOW_STAT("Retracted regular tuples", stats.retracted_regular,
			stats.retracted_regular_by_type[i],
			stats.retracted_regular_by_thread[i]);

  SHOW_STAT("Proved linear tuples", stats.proved_linear,
			stats.proved_linear_by_type[i],
			stats.proved_linear_by_thread[i]);

  SHOW_STAT("Retracted linear tuples", stats.retracted_linear,
			stats.retracted_linear_by_type[i],
			stats.retracted_linear_by_thread[i]);

  SHOW_STAT("Consumed linear tuples", stats.consumed_linear,
			stats.consumed_linear_by_type[i],
			stats.consumed_linear_by_thread[i]);

  SHOW_STAT("Tuples sent", stats.tuples_sent,
			stats.tuples_sent_by_type[i],
			stats.tuples_sent_by_thread[i]);

  SHOW_STAT("Tuples sent to itself", stats.tuples_sent_itself,
			stats.tuples_sent_itself_by_type[i],
			stats.tuples_sent_itself_by_thread[i]);

  SHOW_STAT("Tuples allocated", stats.tuples_allocated,
			stats.tuples_allocated_by_type[i],
			stats.tuples_allocated_by_thread[i]);
  
  fprintf(out, "  Lists allocated: %ld\n", stats.lists_allocated);
  fprintf(out, "    Int: %ld\n", stats.lists_allocated_by_type[LIST_INT]);
  fprintf(out, "    Float: %ld\n", stats.lists_allocated_by_type[LIST_FLOAT]);
  fprintf(out, "    Node: %ld\n", stats.lists_allocated_by_type[LIST_NODE]);
	SHOW_THREADS(stats.lists_allocated, stats.lists_allocated_by_thread[i]);
  fprintf(out, "\n");
  
  fprintf(out, "  Sets allocated: %ld\n", stats.sets_allocated);
  fprintf(out, "    Int: %ld\n", stats.sets_allocated_by_type[SET_INT]);
  fprintf(out, "    Float: %ld\n", stats.sets_allocated_by_type[SET_FLOAT]);
	SHOW_THREADS(stats.sets_allocated, stats.sets_allocated_by_thread[i]);
	fprintf(out, "\n");

	fprintf(out, "  GC executions: %ld\n", stats.run_gc);
	fprintf(out, "  GC time: %.3lf ms\n", stats.time_gc);
	fprintf(out, "\n");

	fprintf(out, "  Lists collected: %ld\n", stats.lists_collected);
	fprintf(out, "    Int: %ld\n", stats.lists_collected_by_type[LIST_INT]);
	fprintf(out, "    Float: %ld\n", stats.lists_collected_by_type[LIST_FLOAT]);
	fprintf(out, "    Node: %ld\n", stats.lists_collected_by_type[LIST_NODE]);
	SHOW_THREADS(stats.lists_collected, stats.lists_collected_by_thread[i]);
	fprintf(out, "\n");
	
	fprintf(out, "  Sets collected: %ld\n", stats.sets_collected);
  fprintf(out, "    Int: %ld\n", stats.sets_collected_by_type[SET_INT]);
  fprintf(out, "    Float: %ld\n", stats.sets_collected_by_type[SET_FLOAT]);
	SHOW_THREADS(stats.sets_collected, stats.sets_allocated_by_thread[i]);
	fprintf(out, "\n");

	fprintf(out, "  Execution time: %.3lfms\n", stats.time_exec);
}
