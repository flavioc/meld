#ifndef STATS_H
#define STATS_H

#include <pthread.h>
#include <stdio.h>

#include "config.h"

#include "api.h"
#include "model.h"
#include "node.h"

typedef enum _stat_list_type {
  LIST_INT,
  LIST_FLOAT,
  LIST_NODE,
  LIST_NUM_TYPES
} stat_list_type;

typedef enum _stat_set_type {
  SET_INT,
  SET_FLOAT,
  SET_NUM_TYPES
} stat_set_type;

typedef struct _stats_data {
	big_counter proved_regular;
	big_counter retracted_regular;
	big_counter *proved_regular_by_type;
	big_counter *proved_regular_by_thread;
	big_counter *retracted_regular_by_type;
	big_counter *retracted_regular_by_thread;
	big_counter proved_linear;
	big_counter retracted_linear;
	big_counter consumed_linear;
	big_counter *proved_linear_by_type;
	big_counter *proved_linear_by_thread;
	big_counter *retracted_linear_by_type;
	big_counter *retracted_linear_by_thread;
	big_counter *consumed_linear_by_type;
	big_counter *consumed_linear_by_thread;
  big_counter tuples_sent;
  big_counter *tuples_sent_by_type;
	big_counter *tuples_sent_by_thread;
  big_counter tuples_sent_itself;
  big_counter *tuples_sent_itself_by_type;
	big_counter *tuples_sent_itself_by_thread;
  big_counter tuples_allocated;
  big_counter *tuples_allocated_by_type;
	big_counter *tuples_allocated_by_thread;
  big_counter lists_allocated;
  big_counter *lists_allocated_by_type;
	big_counter *lists_allocated_by_thread;
  big_counter sets_allocated;
  big_counter *sets_allocated_by_type;
	big_counter *sets_allocated_by_thread;

	big_counter run_gc;
	double time_gc;
	big_counter lists_collected;
	big_counter *lists_collected_by_type;
	big_counter *lists_collected_by_thread;
  big_counter sets_collected;
  big_counter *sets_collected_by_type;
	big_counter *sets_collected_by_thread;
	double time_exec;
	pthread_mutex_t proved_regular_mutex;
	pthread_mutex_t retracted_regular_mutex;
	pthread_mutex_t consumed_linear_mutex;
	pthread_mutex_t proved_linear_mutex;
	pthread_mutex_t retracted_linear_mutex;
  pthread_mutex_t tuples_sent_mutex;
  pthread_mutex_t tuples_sent_itself_mutex;
  pthread_mutex_t tuples_allocated_mutex;
  pthread_mutex_t lists_allocated_mutex;
  pthread_mutex_t sets_allocated_mutex;
	pthread_mutex_t run_gc_mutex;
	pthread_mutex_t lists_collected_mutex;
  pthread_mutex_t sets_collected_mutex;
} stats_data;

void stats_init(void);

void stats_proved_tuple(tuple_type type, ref_count count);
void stats_retracted_tuple(tuple_type type, ref_count count);
void stats_proved_regular(tuple_type type, ref_count count);
void stats_retracted_regular(tuple_type type, ref_count count);
void stats_consumed_linear(tuple_type type);
void stats_proved_linear(tuple_type type, ref_count count);
void stats_retracted_linear(tuple_type type, ref_count count);
void stats_tuples_sent(tuple_type type);
void stats_tuples_sent_itself(tuple_type type);
void stats_tuples_allocated(tuple_type type);
void stats_lists_allocated(stat_list_type type);
void stats_sets_allocated(stat_set_type type);
void stats_run_gc(void);
void stats_lists_collected(stat_list_type type);
void stats_sets_collected(stat_set_type type);
void stats_time_gc(double elapsed);
void stats_end_exec(void);
void stats_start_exec(void);

void stats_print(FILE *out);

static inline void
stats_dump(void)
{
	stats_print(stdout);
}

#endif /* STATS_H */
