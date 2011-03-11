
#include <stdlib.h>

#include "api.h"
#include "core.h"
#include "model.h"

#include "set_runtime.h"
#include "list_runtime.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

//#define DEBUG_INSTRS
//#define DEBUG_ALLOCS
//#define DEBUG_PROVED_TUPLES

static unsigned char **deltas = NULL;
int *delta_sizes = NULL;
unsigned char *arguments = NULL;
tuple_type TYPE_EDGE = -1;
tuple_type TYPE_INIT = -1;
tuple_type TYPE_COLOCATED = -1;
tuple_type TYPE_PROVED = -1;
tuple_type TYPE_TERMINATE = -1;
tuple_type TYPE_TERMINATED = -1;

bool
queue_is_empty(tuple_queue *queue)
{
  return queue->head == NULL;
}

void
queue_push_tuple(tuple_queue *queue, tuple_entry *entry)
{
  if(queue->head == NULL)
    queue->head = queue->tail = entry;
  else {
    queue->tail->next = entry;
    queue->tail = entry;
  }
}

tuple_t
queue_pop_tuple(tuple_queue *queue)
{
  tuple_entry *entry = NULL;
  
  if (queue->head) {
    entry = queue->head;
    queue->head = queue->head->next;
    
    if (queue->head == NULL)
      queue->tail = NULL;
  }
  
  return entry;
}

static tuple_t
queue_dequeue_pos(tuple_queue *queue, tuple_entry **pos)
{
  tuple_entry *entry = *pos;
  tuple_entry *next = (*pos)->next;
  
  if (entry == queue->tail) {
    if(entry == queue->head)
      queue->tail = NULL;
    else
      queue->tail = (tuple_entry *)pos; /* previous */
  }
  
  *pos = next;
    
  tuple_t tuple = entry->tuple;
  meld_free(entry);
  
  return tuple;
}

tuple_entry*
queue_enqueue(tuple_queue *queue, tuple_t tuple, record_type isNew)
{
  tuple_entry *entry = meld_malloc(sizeof(tuple_entry));
  
  entry->tuple = tuple;
  entry->records = isNew;
  entry->next = NULL;
  
  queue_push_tuple(queue, entry);

  return entry;
}

tuple_t
queue_dequeue(tuple_queue *queue, ref_count *isNew)
{
  tuple_entry *entry = queue_pop_tuple(queue);

  tuple_t tuple = entry->tuple;

  if(isNew)
    *isNew = entry->records.count;

  meld_free(entry);

  return tuple;
}

tuple_pentry*
p_dequeue(tuple_pqueue *q)
{
  tuple_pentry *ret = q->queue;
  
  if(q->queue != NULL)
    q->queue = q->queue->next;
    
  return ret;
}

void
p_enqueue(tuple_pqueue *queue, meld_int priority, tuple_t tuple,
		void *rt, record_type isNew)
{
  tuple_pentry *entry = meld_malloc(sizeof(tuple_pentry));

  entry->tuple = tuple;
  entry->records = isNew;
  entry->priority = priority;
  entry->rt = rt;

  tuple_pentry **spot;
  for (spot = &(queue->queue);
       *spot != NULL &&
         (*spot)->priority < priority;
    spot = &((*spot)->next));

  entry->next = *spot;
  *spot = entry;
}
    
void
init_deltas(void)
{
	int i;

	deltas = (unsigned char **)meld_malloc(sizeof(unsigned char*)*NUM_TYPES);
	delta_sizes = (int *)meld_malloc(sizeof(int)*NUM_TYPES);

	for (i = 0; i < NUM_TYPES; ++i) {
		delta_sizes[i] = TYPE_NODELTAS(i);
		deltas[i] = (unsigned char*)TYPE_DELTAS(i);
	}
}

static int type;
void
init_fields(void)
{
  size_t total = 2*NUM_TYPES;
  int i, j;

  for(i = 0; i < NUM_TYPES; ++i) {
    total += TYPE_NOARGS(i) * 2;
	}
  
  arguments = meld_malloc(total);
  unsigned char *start = arguments + 2*NUM_TYPES;
  unsigned char offset, size;
  
  for(i = 0; i < NUM_TYPES; ++i) {
    arguments[i*2] = start - arguments; /* start */
    offset = 0;
    
    for(j = 0; j < TYPE_NOARGS(i); ++j) {
      type = TYPE_ARG_TYPE(i, j);
      switch (type) {

      case (int)FIELD_INT:
      case (int)FIELD_TYPE:
				size = sizeof(meld_int);
				break;

      case (int)FIELD_FLOAT:
				size = sizeof(meld_float);
				break;

      case (int)FIELD_ADDR:
      case (int)FIELD_LIST_INT:
      case (int)FIELD_LIST_FLOAT:
      case (int)FIELD_LIST_ADDR:
      case (int)FIELD_SET_INT:
      case (int)FIELD_SET_FLOAT:
				size = sizeof(void*);
				break;

      default:
				assert(0);
				size = 0;
				break;

      }
      
      start[0] = size; /* argument size */
      start[1] = offset; /* argument offset */
      
      offset += size;
      start += 2;
    }
    
    arguments[i*2+1] = offset + TYPE_FIELD_SIZE; /* tuple size */
  }
}

void init_consts(void)
{
	tuple_type i;
	for (i = 0; i < NUM_TYPES; i++) {
		if (strcmp(TYPE_NAME(i), "_init") == 0)
			TYPE_INIT = i;
		else if(strcmp(TYPE_NAME(i), "edge") == 0)
			TYPE_EDGE = i;
		else if(strcmp(TYPE_NAME(i), "colocated") == 0)
			TYPE_COLOCATED = i;
		else if(strcmp(TYPE_NAME(i), "proved") == 0)
      TYPE_PROVED = i;
		else if(strcmp(TYPE_NAME(i), "terminate") == 0)
      TYPE_TERMINATE = i;
	}	
}

static inline
pcounter advance(pcounter pc)
{
	if (SEND(pc)) {
		int count = 3;
		
		if (VAL_IS_FLOAT(SEND_ARG1(pc)))
      count += sizeof(meld_float);
    else if (VAL_IS_INT(SEND_ARG1(pc)))
      count += sizeof(meld_int);
    else if (VAL_IS_FIELD(SEND_ARG1(pc)))
      count += 2;
		else if (VAL_IS_REVERSE(SEND_ARG1(pc)))
			count += 2;
		else {
			assert(0);
			exit(1);
		}

		return pc+count;
	}
	else if(OP(pc)) {
		int count = 3;

    /* check v1 */
		if (VAL_IS_FLOAT(OP_ARG1(pc)))
			count += sizeof(meld_float);
		else if (VAL_IS_INT(OP_ARG1(pc)))
  		count += sizeof(meld_int);
		else if (VAL_IS_FIELD(OP_ARG1(pc)))
			count += 2;
		else if (VAL_IS_REVERSE(OP_ARG1(pc)))
			count += 2;
		else if (VAL_IS_REG(OP_ARG1(pc)))
		  {} /* do nothing */
		else {
			assert(0);
			exit(1);
		}
			
		/* check v2 */
		if (VAL_IS_FLOAT(OP_ARG2(pc)))
			count += sizeof(meld_float);
	  else if (VAL_IS_INT(OP_ARG2(pc)))
			count += sizeof(meld_int);
		else if (VAL_IS_FIELD(OP_ARG2(pc)))
			count += 2;
		else if (VAL_IS_REVERSE(OP_ARG2(pc)))
			count += 2;
	  else if (VAL_IS_REG(OP_ARG2(pc)))
	    {} /* do nothing */
		else {
			assert(0);
			exit(1);
		}

		return pc+count;
	}
	else if(MOVE(pc)) {
		int count = 2;
		
		/* move src */
		if (VAL_IS_FLOAT(MOVE_SRC(pc)))
			count += sizeof(meld_float);
		else if (VAL_IS_INT(MOVE_SRC(pc)))
  		count += sizeof(meld_int);
  	else if (VAL_IS_FIELD(MOVE_SRC(pc)))
  		count += 2;
		else if (VAL_IS_REVERSE(MOVE_SRC(pc)))
			count += 2;
		else if (VAL_IS_TUPLE(MOVE_SRC(pc)))
			{} /* nothing */
		else if(VAL_IS_REG(MOVE_SRC(pc)))
			{} /* nothing */
		else if(VAL_IS_HOST(MOVE_SRC(pc)))
			{} /* nothing */
		else {
			assert(0);
			exit(1);
		}
  		
  	/* move dst */
		if (VAL_IS_FLOAT(MOVE_DST(pc)))
			count += sizeof(meld_float);
	  else if (VAL_IS_INT(MOVE_DST(pc)))
			count += sizeof(meld_int);
		else if (VAL_IS_FIELD(MOVE_DST(pc)))
			count += 2;
		else if (VAL_IS_REVERSE(MOVE_DST(pc)))
			count += 2;
		else if (VAL_IS_REG(MOVE_DST(pc)))
			{} /* nothing */
		else {
			assert(0);
			exit(1);
		}

		return pc+count;
	}
	else if(ITER(pc)) {
    
    pc += ITER_BASE;
    
    if(ITER_MATCH_NONE(pc))
      pc += 2;
    else {
      pcounter old;
      while(1) {
        old = pc;

			  if (VAL_IS_FLOAT(ITER_MATCH_VAL(pc)))
				  pc += sizeof(meld_float);
			  else if (VAL_IS_INT(ITER_MATCH_VAL(pc)))
				  pc += sizeof(meld_int);
			  else if (VAL_IS_FIELD(ITER_MATCH_VAL(pc)))
				  pc += 2;
				else if (VAL_IS_REVERSE(ITER_MATCH_VAL(pc)))
					pc += 2;
				else {
					assert(0);
					exit(1);
				}

        pc += 2;

        if(ITER_MATCH_END(old))
          break;
      }
    }

		return pc;
	} else if (ALLOC(pc)) {
		int count = 2;

		if (VAL_IS_INT(ALLOC_DST(pc)))
			count += sizeof(meld_int);
		else if	(VAL_IS_FLOAT(ALLOC_DST(pc)))
			count += sizeof(meld_float);
		else if (VAL_IS_FIELD(ALLOC_DST(pc)))
			count += 2;
		else if (VAL_IS_REG(ALLOC_DST(pc)))
			{} /* nothing */
		else if (VAL_IS_REVERSE(ALLOC_DST(pc))) {
			count += 2;
			assert(0);
			exit(1);
		} else {
			assert(0);
			exit(1);
		}

		return pc+count;
	}
	else if (CALL(pc)) {
		int numArgs = CALL_ARGS(pc);
		int i;
		
		for (i = 0, pc+=2; i < numArgs; i++, pc++) {
			if (VAL_IS_FLOAT(CALL_VAL(pc)))
        pc += sizeof(meld_float);
			else if (VAL_IS_INT(CALL_VAL(pc)))
				pc += sizeof(meld_int);
			else if (VAL_IS_FIELD(CALL_VAL(pc)))
				pc += 2;
			else if (VAL_IS_REVERSE(CALL_VAL(pc))) {
				pc += 2;
				assert(0);
				exit(1);
			}
		}
		return pc;
	}
	else if (IF(pc)) {
		return pc+IF_BASE;
	}
	else {
		return pc+1;
	}
}

static inline
anything eval_dst(const unsigned char value,
  pcounter *pc, Register *reg, size_t *size)
{
  if (VAL_IS_REG(value)) {
		*size = sizeof(Register);
		return &(reg)[VAL_REG(value)];
	} else if (VAL_IS_FIELD(value)) {
		int reg_index = VAL_FIELD_REG(*pc);
		int field_num = VAL_FIELD_NUM(*pc);
		tuple_t tuple = (tuple_t)reg[reg_index];
		tuple_type type = TUPLE_TYPE(tuple);

		*size = TYPE_ARG_SIZE(type, field_num);

		(*pc) += 2;

		return GET_TUPLE_FIELD(tuple, field_num);
	} else if (VAL_IS_INT(value)) {
    assert(0);
	} else if (VAL_IS_FLOAT(value)) {
    assert(0);
	} else if(VAL_IS_TUPLE(value)) {
    assert(0);
	} else if(VAL_IS_HOST(value)) {
    assert(0);
  } else {
		assert(0 /* invalid value */ );
	}
}

static inline
anything eval(const unsigned char value, tuple_t tuple,
  pcounter *pc, Register *reg)
{
  if (VAL_IS_HOST(value)) {
    return (anything)EVAL_HOST;
  } else if (VAL_IS_REG(value)) {
	  return (anything)&(reg[VAL_REG(value)]);
	} else if (VAL_IS_TUPLE(value)) {
		return (anything)tuple;
	} else if (VAL_IS_FIELD(value)) {
		const unsigned char reg_index = VAL_FIELD_REG(*pc);
		const unsigned char field_num = VAL_FIELD_NUM(*pc);
		tuple_t tuple = (tuple_t)reg[reg_index];
		(*pc) += 2;
		
    return GET_TUPLE_FIELD(tuple, field_num);
	} else if (VAL_IS_INT(value)) {
    anything ret = (anything)(*pc);
		*pc = *pc + sizeof(meld_int);
    return ret;
	} else if (VAL_IS_FLOAT(value)) {
    anything ret = (anything)(*pc);
    
    *pc = *pc + sizeof(meld_float);
	
    return ret;
	} else if (VAL_IS_REVERSE(value)) {
		const int reg_index = VAL_FIELD_REG(*pc);
		const int field_num = VAL_FIELD_NUM(*pc);
		tuple_t tuple = (tuple_t)reg[reg_index];

		(*pc) += 2;

#ifdef PARALLEL_MACHINE
		List *route = MELD_LIST(GET_TUPLE_FIELD(tuple, field_num));
		List *clone = list_copy(route);

		list_reverse_first(clone);

		thread_self()->reverse_list = clone;
    
    return (anything)&thread_self()->reverse_list;
#else
		return GET_TUPLE_FIELD(tuple, field_num);
#endif /* PARALLEL_MACHINE */
  } else {
		assert(0 /* invalid value */ );
	}
}

static inline
bool aggregate_accumulate(aggregate_type agg_type, anything acc, anything obj, int count)
{
	switch (agg_type) {
		case AGG_SET_UNION_INT: {
				Set *set = MELD_SET(acc);
				set_int_insert(set, MELD_INT(obj));
				set_print(set);
				return false;
			}
		case AGG_SET_UNION_FLOAT: {
				Set *set = MELD_SET(acc);
				set_float_insert(set, MELD_FLOAT(obj));
				set_print(set);
				return false;
			}

		case AGG_FIRST:
      return false;

		case AGG_MAX_INT:
			if (MELD_INT(obj) > MELD_INT(acc)) {
				MELD_INT(acc) = MELD_INT(obj);
        return true;
      } else
        return false;

		case AGG_MIN_INT:
			if (MELD_INT(obj) < MELD_INT(acc)) {
				MELD_INT(acc) = MELD_INT(obj);
        return true;
      } else
        return false;
		
		case AGG_SUM_INT:
			MELD_INT(acc) += MELD_INT(obj) * count;
      return false;
			
		case AGG_MAX_FLOAT:
			if(MELD_FLOAT(obj) > MELD_FLOAT(acc)) {
				MELD_FLOAT(acc) = MELD_FLOAT(obj);
        return true;
      } else
        return false;
    
    case AGG_MIN_FLOAT:
			if(MELD_FLOAT(obj) < MELD_FLOAT(acc)) {
				MELD_FLOAT(acc) = MELD_FLOAT(obj);
        return true;
      } else
        return false;
    
    case AGG_SUM_FLOAT:
			MELD_FLOAT(acc) += MELD_FLOAT(obj) * (meld_float)count;
      return false;

		case AGG_SUM_LIST_INT: {
			List *result_list = MELD_LIST(acc);
			List *other_list = MELD_LIST(obj);

			if(list_total(result_list) != list_total(other_list)) {
				fprintf(stderr, "lists differ in size for accumulator AGG_SUM_LIST_INT:"
						 " %d vs %d\n", list_total(result_list), list_total(other_list));
				exit(1);
			}

			list_iterator it_result = list_get_iterator(result_list);
			list_iterator it_other = list_get_iterator(other_list);

			while(list_iterator_has_next(it_result)) {
				list_iterator_int(it_result) += list_iterator_int(it_other) * (meld_int)count;

				it_other = list_iterator_next(it_other);
				it_result = list_iterator_next(it_result);
			}
			
			return false;
	  }
	  
		case AGG_SUM_LIST_FLOAT: {
			List *result_list = MELD_LIST(acc);
			List *other_list = MELD_LIST(obj);

			if(list_total(result_list) != list_total(other_list)) {
				fprintf(stderr, "lists differ in size for accumulator AGG_SUM_LIST_FLOAT: "
						"%d vs %d\n", list_total(result_list), list_total(other_list));
				exit(1);
			}

			list_iterator it_result = list_get_iterator(result_list);
			list_iterator it_other = list_get_iterator(other_list);

			while(list_iterator_has_next(it_result)) {
				list_iterator_float(it_result) += list_iterator_float(it_other) * (meld_float)count;

				it_result = list_iterator_next(it_result);
				it_other = list_iterator_next(it_other);
			}
			
			return false;
	  }
   }

	assert(0);
	while(1);
}

static inline bool
aggregate_changed(aggregate_type agg_type, anything v1, anything v2)
{
  switch(agg_type) {
    case AGG_FIRST:
      return false;

    case AGG_MIN_INT:
    case AGG_MAX_INT:
    case AGG_SUM_INT:
			return MELD_INT(v1) != MELD_INT(v2);
    
    case AGG_MIN_FLOAT:
    case AGG_MAX_FLOAT:
    case AGG_SUM_FLOAT:
			return MELD_FLOAT(v1) != MELD_FLOAT(v2);

		case AGG_SET_UNION_INT:
		case AGG_SET_UNION_FLOAT: {
				Set *setOld = MELD_SET(v1);
				Set *setNew = MELD_SET(v2);

				if(!set_equal(setOld, setNew))
					return true;

				/* delete new set union */
				set_delete(setNew);
				return false;
			}
			break;

		case AGG_SUM_LIST_INT:
		case AGG_SUM_LIST_FLOAT: {
				List *listOld = MELD_LIST(v1);
				List *listNew = MELD_LIST(v2);

				if(!list_equal(listOld, listNew))
					return true;

				/* delete new list */
				list_delete(listNew);
				return false;
			}
			break;

    default:
			assert(0);
      return true;
  }

	assert(0);
	while(1);
}

static inline void
aggregate_seed(aggregate_type agg_type, anything acc, anything start, int count, size_t size)
{
	switch(agg_type) {
		case AGG_FIRST:
			memcpy(acc, start, size);
			return;
		case AGG_MIN_INT:
		case AGG_MAX_INT:
			MELD_INT(acc) = MELD_INT(start);
			return;
		case AGG_SUM_INT:
			MELD_INT(acc) = MELD_INT(start) * count;
			return;
		case AGG_MIN_FLOAT:
		case AGG_MAX_FLOAT:
			MELD_FLOAT(acc) = MELD_FLOAT(start);
			return;
		case AGG_SUM_FLOAT:
			MELD_FLOAT(acc) = MELD_FLOAT(start) * count;
			return;
		case AGG_SET_UNION_INT: {
				Set *set = set_int_create();
				set_int_insert(set, MELD_INT(start));
				set_print(set);
				MELD_SET(acc) = set;
				return;
			}
		case AGG_SET_UNION_FLOAT: {
				Set *set = set_float_create();
				set_float_insert(set, MELD_FLOAT(start));
				set_print(set);
				MELD_SET(acc) = set;
				return;
			}
		case AGG_SUM_LIST_INT: {
				List *result_list = list_int_create();
				List *start_list = MELD_LIST(start);

				/* add values to result_list */
				list_iterator it;
				for(it = list_get_iterator(start_list); list_iterator_has_next(it);
						it = list_iterator_next(it))
				{
					meld_int total = list_iterator_int(it) * (meld_int)count;
					list_int_push_tail(result_list, total);
				}

				MELD_LIST(acc) = result_list;
				return;
		  }
		case AGG_SUM_LIST_FLOAT: {
				List *result_list = list_float_create();
				List *start_list = MELD_LIST(start);

				/* add values to result_list */
				list_iterator it;
			 	for(it = list_get_iterator(start_list); list_iterator_has_next(it);
						it = list_iterator_next(it))
				{
					meld_float total = list_iterator_float(it) * (meld_float)count;
					list_float_push_tail(result_list, total);
				}

				MELD_LIST(acc) = result_list;
				return;
			}
	}

	assert(0);
	while(1);
}

static inline void
aggregate_free(tuple_t tuple, aggregate_t field_aggregate,
	aggregate_type type_aggregate)
{
	switch(type_aggregate) {
		case AGG_FIRST:
		case AGG_MIN_INT:
		case AGG_MAX_INT:
		case AGG_SUM_INT:
		case AGG_MIN_FLOAT:
		case AGG_MAX_FLOAT:
		case AGG_SUM_FLOAT:
			/* nothing to do */
			break;

		case AGG_SET_UNION_INT:
		case AGG_SET_UNION_FLOAT:
			set_delete(MELD_SET(GET_TUPLE_FIELD(tuple, field_aggregate)));
			break;

		case AGG_SUM_LIST_INT:
		case AGG_SUM_LIST_FLOAT:
			list_delete(MELD_LIST(GET_TUPLE_FIELD(tuple, field_aggregate)));
			break;

		default:
			assert(0);
			break;
	}
}

static inline
void aggregate_recalc(tuple_entry *agg, Register *reg, tuple_type type,
		aggregate_t type_aggregate, bool first_run)
{
  aggregate_type agg_type = AGG_AGG(type_aggregate);
	aggregate_field agg_field = AGG_FIELD(type_aggregate);
  tuple_queue *agg_queue = agg->records.agg_queue;
  tuple_entry *agg_list = agg_queue->head;
  tuple_t tuple = agg_list->tuple;
  
	field_t start = GET_TUPLE_FIELD(tuple, agg_field);

	/* make copy */
	size_t size = TYPE_ARG_SIZE(type, agg_field);
	anything accumulator = meld_malloc(size);

	aggregate_seed(agg_type, accumulator, start, agg_list->records.count, size);
	
	/* calculate offsets to copy right side to aggregated tuple */
	size_t size_offset = TYPE_FIELD_SIZE + TYPE_ARG_OFFSET(type, agg_field) + TYPE_ARG_SIZE(type, agg_field);
	size_t total_copy = TYPE_SIZE(type) - size_offset;
  tuple_t target_tuple = NULL;
  
  if (total_copy > 0)
    target_tuple = tuple;

	tuple_entry *cur = agg_list->next;
	for (; cur != NULL; cur = cur->next) {
		if(aggregate_accumulate(agg_type, accumulator,
		    GET_TUPLE_FIELD(cur->tuple, agg_field), cur->records.count))
      target_tuple = cur->tuple;
	}

	field_t acc_area = GET_TUPLE_FIELD(agg->tuple, agg_field);

	if(first_run)
    memcpy(acc_area, accumulator, size);
	else if (aggregate_changed(agg_type, acc_area, accumulator)) {
		tuple_process(agg->tuple, TYPE_START(type), -1, reg);
		aggregate_free(agg->tuple, agg_field, agg_type);
		memcpy(acc_area, accumulator, size);
		if (total_copy > 0) /* copy right side from target tuple */
      memcpy(((unsigned char *)agg->tuple) + size_offset, ((unsigned char *)target_tuple) + size_offset, total_copy);
		tuple_process(agg->tuple, TYPE_START(type), 1, reg);
	}

	meld_free(accumulator);
}

static inline
void process_deltas(tuple_t tuple, tuple_type type, Register *reg)
{
  tuple_t old = OLDTUPLES[type];
  
  if(old == NULL)
    return;
    
  OLDTUPLES[type] = NULL;

	int i;
	for(i = 0; i < DELTA_TOTAL(type); ++i) {
		int delta_type = DELTA_TYPE(type, i);
		int delta_pos = DELTA_POSITION(type, i);
		tuple_t delta_tuple = ALLOC_TUPLE(TYPE_SIZE(delta_type));

		memcpy(delta_tuple, old, TYPE_SIZE(delta_type));
		TUPLE_TYPE(delta_tuple) = delta_type;

		field_t field_delta = GET_TUPLE_FIELD(delta_tuple, delta_pos);
		field_t field_old = GET_TUPLE_FIELD(old, delta_pos);
		field_t field_new = GET_TUPLE_FIELD(tuple, delta_pos);

		switch(TYPE_ARG_TYPE(type, delta_pos)) {
			case FIELD_INT:
				MELD_INT(field_delta) = MELD_INT(field_new) - MELD_INT(field_old);
				break;
			case FIELD_FLOAT:
				MELD_FLOAT(field_delta) = MELD_FLOAT(field_new) - MELD_FLOAT(field_old);
				break;
			default:
				assert(0);
				break;
		}

		tuple_process(delta_tuple, TYPE_START(TUPLE_TYPE(delta_tuple)), 1, reg);
	}

	FREE_TUPLE(old);
}

static inline tuple_t
tuple_build_proved(tuple_type type, meld_int total)
{
  tuple_t tuple = tuple_alloc(TYPE_PROVED);
  meld_int type_int = (meld_int)type;
  
  SET_TUPLE_FIELD(tuple, 0, &type_int);
  SET_TUPLE_FIELD(tuple, 1, &total);
  
  return tuple;
}

void tuple_do_handle(tuple_type type,	tuple_t tuple, ref_count isNew, Register *reg)
{
  if(TYPE_IS_PROVED(type)) {
    PROVED[type] += (meld_int)isNew;
    tuple_t proved = tuple_build_proved(type, PROVED[type]);
#ifdef DEBUG_PROVED_TUPLES
    printf("New proved for tuple %s: %d\n", tuple_names[type], PROVED[type]);
#endif
    PUSH_NEW_TUPLE(proved);
  } else if(type == TYPE_PROVED) {
    tuple_process(tuple, TYPE_START(type), isNew, reg);
    FREE_TUPLE(tuple);
    return;
  } else if(type == TYPE_TERMINATE) {
    FREE_TUPLE(tuple);
    TERMINATE_CURRENT();
    return;
  } else if(type == TYPE_TERMINATED) {
		tuple_process(tuple, TYPE_START(type), isNew, reg);
		FREE_TUPLE(tuple);
		return;
	}
  
  if(!TYPE_IS_AGG(type) && TYPE_IS_PERSISTENT(type)) {
    persistent_set *persistent = &PERSISTENT[type];
    int i;
    int size = TYPE_SIZE(type);
    
    if(isNew < 0) {
      fprintf(stderr, "meld: persistent types can't be deleted\n");
      exit(EXIT_FAILURE);
    }
    
    for(i = 0; i < persistent->current; ++i) {
      tuple_t stored_tuple = persistent->array + i * size;
      
      if(memcmp(stored_tuple, tuple, size) == 0) {
        FREE_TUPLE(tuple);
        return;
      }
    }
    
    /* new tuple */
    if(persistent->total == persistent->current) {
      if(persistent->total == 0)
        persistent->total = PERSISTENT_INITIAL;
      else
        persistent->total *= 2;
        
      persistent->array = meld_realloc(persistent->array, size * persistent->total);
    }
    
    memcpy(persistent->array + persistent->current * size, tuple, size);
    ++persistent->current;
    
    tuple_process(tuple, TYPE_START(type), isNew, reg);
    
    return;
  }
  
	if (!TYPE_IS_AGG(type) || TYPE_IS_LINEAR(type))
	{
    tuple_queue *queue = &TUPLES[type];
		tuple_entry** current;
    tuple_entry* cur;
		
		for (current = &queue->head;
			 *current != NULL;
			 current = &(*current)->next)
		{
      cur = *current;

			if (memcmp(cur->tuple,
						 tuple,
						 TYPE_SIZE(type)) == 0)
			{
				cur->records.count += isNew;

				if (cur->records.count <= 0) {
					/* only process if it isn't linear */
					if (!TYPE_IS_LINEAR(type)) {
						tuple_process(tuple, TYPE_START(TUPLE_TYPE(tuple)), -1, reg);
						FREE_TUPLE(queue_dequeue_pos(queue, current));
					} else {
						if(DELTA_WITH(type)) {
						  if(OLDTUPLES[type])
                FREE_TUPLE(OLDTUPLES[type]);
                
              OLDTUPLES[type] = queue_dequeue_pos(queue, current);
						} else
							FREE_TUPLE(queue_dequeue_pos(queue, current));
					}
				}

				FREE_TUPLE(tuple);

				return;
			}
		}

		// if deleting, return
		if (isNew <= 0) {
			FREE_TUPLE(tuple);
			return;
		}

    queue_enqueue(queue, tuple, (record_type) isNew);

		if(TYPE_IS_LINEAR(type) && DELTA_WITH(type))
			process_deltas(tuple, type, reg);

		tuple_process(tuple, TYPE_START(TUPLE_TYPE(tuple)), isNew, reg);

		return;
	}

  aggregate_t type_aggregate = TYPE_AGGREGATE(type);
	aggregate_field field_aggregate = AGG_FIELD(type_aggregate);

	tuple_entry **current;
  tuple_entry *cur;
  tuple_queue *queue = &(TUPLES[type]);
	
	for (current = &queue->head;
		 (*current) != NULL;
		 current = &(*current)->next)
	{
    cur = *current;
    
		size_t size_begin = TYPE_FIELD_SIZE + TYPE_ARG_OFFSET(type, field_aggregate);
		char *start = (char*)(cur->tuple);

		if(memcmp(start, tuple, size_begin))
			continue;

    tuple_queue *agg_queue = cur->records.agg_queue;

		/* AGG_FIRST aggregate optimization */
		if(AGG_AGG(type_aggregate) == AGG_FIRST
				&& isNew > 0
				&& !queue_is_empty(agg_queue))
		{
			FREE_TUPLE(tuple);
			return;
		}

		tuple_entry** current2;
    tuple_entry* cur2;
		
		for (current2 = &agg_queue->head;
			 *current2 != NULL;
			 current2 = &(*current2)->next)
		{
      cur2 = *current2;

			if (memcmp(cur2->tuple, tuple, TYPE_SIZE(type)) == 0)
			{
				cur2->records.count += isNew;

				if (cur2->records.count <= 0) {
					// remove it
					FREE_TUPLE(queue_dequeue_pos(agg_queue, current2));

					if (queue_is_empty(agg_queue)) {
						/* aggregate is removed */
						tuple_t agg_tuple = queue_dequeue_pos(queue, current);
						
						/* delete queue */
            meld_free(agg_queue);

						tuple_process(agg_tuple, TYPE_START(type), -1, reg);
						aggregate_free(agg_tuple, field_aggregate, AGG_AGG(type_aggregate));
						FREE_TUPLE(agg_tuple);
					} else
						aggregate_recalc(cur, reg, type, type_aggregate, false);
				} else
					aggregate_recalc(cur, reg, type, type_aggregate, false);

				FREE_TUPLE(tuple);
				return;
			}
		}

		// if deleting, return
		if (isNew <= 0) {
			FREE_TUPLE(tuple);
			return;
		}

    queue_enqueue(agg_queue, tuple, (record_type) isNew);
		aggregate_recalc(cur, reg, type, type_aggregate, false);
		
		return;
	}

	/* if deleting, return */
	if (isNew <= 0) {
		FREE_TUPLE(tuple);
		return;
	}

	/* so now we know we have a new tuple */
	tuple_t tuple_cpy = ALLOC_TUPLE(TYPE_SIZE(type));
	memcpy(tuple_cpy, tuple, TYPE_SIZE(type));

  /* create aggregate queue */
  tuple_queue *agg_queue = meld_malloc(sizeof(tuple_queue));
  
  queue_init(agg_queue);
  
  queue_enqueue(agg_queue, tuple, (record_type) isNew);
	tuple_entry *entry =
	  queue_enqueue(&TUPLES[type], tuple_cpy, (record_type)agg_queue);

	aggregate_recalc(entry, reg, type, type_aggregate, true);
	tuple_process(tuple, TYPE_START(type), isNew, reg);
}

int
queue_length (tuple_queue *queue)
{
	int i;
  tuple_entry *entry = queue->head;
  
	for (i = 0; entry != NULL; entry = entry->next, i++);

	return i;
}

int tuple_process(tuple_t tuple, pcounter pc, ref_count isNew, Register *reg)
{
  for (; ; pc = advance(pc)) {
eval_loop: /* for jump instructions */
		if (RETURN(pc)) {
#ifdef DEBUG_INSTRS
        printf("RETURN\n");
#endif
			return RET_RET;
		}

		/* perform an instruction */
		if (IF(pc)) {
#ifdef DEBUG_INSTRS
        printf("IF reg %d ", IF_REG(pc));
#endif
			if (!reg[IF_REG(pc)]) {
#ifdef DEBUG_INSTRS
          printf("no\n");
#endif
				pc += IF_JUMP(pc);
				goto eval_loop;
			}

#ifdef DEBUG_INSTRS
			printf("yes\n");
#endif
		} else if (ELSE(pc)) {
			fprintf(stderr, "ELSE NOT IMPLEMENTED YET!\n");
			assert(0);
		} else if (ITER(pc)) {

			const tuple_type type = ITER_TYPE(pc);
			int i, length;
      tuple_t *list;
			pcounter jump = pc + ITER_JUMP(pc);
      int size = TYPE_SIZE(type);
			
			/* produce a random ordering for all tuples of the appropriate type */
			
			if(TYPE_IS_PERSISTENT(type) && !TYPE_IS_AGG(type)) {
			  /* persistent aggregate types not supported */
        persistent_set *persistent = &PERSISTENT[type];
        
        length = persistent->current;
        list = meld_malloc(sizeof(tuple_t) * length);

        for(i = 0; i < length; i++) {
          int j = random() % (i + 1);
          
          list[i] = list[j];
          list[j] = persistent->array + i * size;
        }
		  } else {
		    /* non-persistent type */
  			tuple_entry *entry = TUPLES[type].head;
		    
		    length = queue_length(&TUPLES[ITER_TYPE(pc)]);
		    list = meld_malloc(sizeof(tuple_t) * length);
		    
			  for (i = 0; i < length; i++) {
				  int j = random() % (i+1);

				  list[i] = list[j];
				  list[j] = entry->tuple;

				  entry = entry->next;
			  }
		  }
			
#ifdef DEBUG_INSTRS
        printf("ITER %s len=%d\n", tuple_names[type], length);
#endif

      if(length == 0) {
        /* no need to execute any further code, just jump! */
        pc = jump;
  			goto eval_loop;
      }

			/* iterate over all tuples of the appropriate type */
      tuple_t next_tuple;
      
			for (i = 0; i < length; i++) {
				next_tuple = list[i];

				bool matched = 1;
				pcounter tmppc;

        tmppc = pc + ITER_BASE;

        if(!ITER_MATCH_NONE(tmppc)) {
				  /* check to see if it matches */
          while (1) {
            pcounter old_pc = tmppc + 2;
					  const unsigned char fieldnum = ITER_MATCH_FIELD(tmppc);
					  const unsigned char type_size = TYPE_ARG_SIZE(type, fieldnum);

            field_t field = GET_TUPLE_FIELD(next_tuple, fieldnum);
            anything val = eval(ITER_MATCH_VAL(tmppc), &tuple, &old_pc, reg);
            
            matched = matched && (memcmp(field, val, type_size) == 0);

            if(ITER_MATCH_END(tmppc))
              break;

            tmppc = old_pc;
          }
				}

#ifdef DEBUG_INSTRS
				printf("MATCHED: %d %d\n", matched, length);
#endif
          
				if (matched) {
					if (RET_RET == tuple_process(next_tuple, advance(pc), isNew, reg)) {
						meld_free(list);
						return RET_RET;
					}
				}
			}

			meld_free(list);

			/* advance the pc to the end of the loop */
			pc = jump;
			goto eval_loop;

		} else if (NEXT(pc)) {
#ifdef DEBUG_INSTRS
        printf("NEXT\n");
#endif
			return RET_NEXT;
		} else if (SEND(pc)) {
			pcounter old_pc = pc+3;
			Register send_reg = reg[SEND_MSG(pc)];
			Register send_rt = reg[SEND_RT(pc)];

#ifdef DEBUG_INSTRS
			printf("SEND\n");
#endif

			tuple_send((tuple_t)send_reg, (void*)send_rt,
			    MELD_INT(eval(SEND_DELAY(pc), &tuple, &old_pc, reg)), isNew);
		} else if (REMOVE(pc)) {
			if (isNew > 0) {
        int reg_remove = REMOVE_REG(pc);
				tuple_t tuple = (tuple_t)reg[reg_remove];
				tuple_type type = TUPLE_TYPE(tuple);
				int size = TYPE_SIZE(type);

				tuple_handle(memcpy(meld_malloc(size), (void *)reg[reg_remove], size), -1, reg);
				reg[REMOVE_REG(pc)] = 0;

#ifdef STATISTICS
				if(TYPE_IS_LINEAR(type))
					stats_consumed_linear(type);
#endif
			}	
		} else if (OP(pc)) {

			pcounter old_pc = pc+3;
			
#ifdef DEBUG_INSTRS
			printf("OP to %d\n", OP_DST(pc));
#endif

      Register *arg1, *arg2;
      
      arg1 = eval(OP_ARG1(pc), &tuple, &old_pc, reg);
      arg2 = eval(OP_ARG2(pc), &tuple, &old_pc, reg);
      
      Register *dest = reg + OP_DST(pc);
      
      switch(OP_OP(pc)) {
        case OP_NEQI: *dest = (MELD_INT(arg1) != MELD_INT(arg2)); break;
        case OP_EQI: *dest = (MELD_INT(arg1) == MELD_INT(arg2)); break;
        case OP_LESSI: *dest = (MELD_INT(arg1) < MELD_INT(arg2)); break;
        case OP_LESSEQI: *dest = (MELD_INT(arg1) <= MELD_INT(arg2)); break;
        case OP_GREATERI: *dest = (MELD_INT(arg1) > MELD_INT(arg2)); break;
        case OP_GREATEREQI: *dest = (MELD_INT(arg1) >= MELD_INT(arg2)); break;
        case OP_MODI: MELD_INT(dest) = (MELD_INT(arg1) % MELD_INT(arg2)); break;
        case OP_PLUSI: MELD_INT(dest) = (MELD_INT(arg1) + MELD_INT(arg2)); break;
        case OP_MINUSI: MELD_INT(dest) = (MELD_INT(arg1) - MELD_INT(arg2)); break;
        case OP_TIMESI: MELD_INT(dest) = (MELD_INT(arg1) * MELD_INT(arg2)); break;
        case OP_DIVI: MELD_INT(dest) = (MELD_INT(arg1) / MELD_INT(arg2)); break;
        case OP_NEQF: *dest = (MELD_FLOAT(arg1) != MELD_FLOAT(arg2)); break;
        case OP_EQF: *dest = (MELD_FLOAT(arg1) == MELD_FLOAT(arg2)); break;
        case OP_LESSF: *dest = (MELD_FLOAT(arg1) < MELD_FLOAT(arg2)); break;
        case OP_LESSEQF: *dest = (MELD_FLOAT(arg1) <= MELD_FLOAT(arg2)); break;
        case OP_GREATERF: *dest = (MELD_FLOAT(arg1) > MELD_FLOAT(arg2)); break;
        case OP_GREATEREQF: *dest = (MELD_FLOAT(arg1) >= MELD_FLOAT(arg2)); break;
        case OP_MODF: MELD_FLOAT(dest) = fmod(MELD_FLOAT(arg1), MELD_FLOAT(arg2)); break;
        case OP_PLUSF: MELD_FLOAT(dest) = (MELD_FLOAT(arg1) + MELD_FLOAT(arg2)); break;
        case OP_MINUSF: MELD_FLOAT(dest) = (MELD_FLOAT(arg1) - MELD_FLOAT(arg2)); break;
        case OP_TIMESF: MELD_FLOAT(dest) = (MELD_FLOAT(arg1) * MELD_FLOAT(arg2)); break;
        case OP_DIVF: MELD_FLOAT(dest) = (MELD_FLOAT(arg1) / MELD_FLOAT(arg2)); break;
        case OP_NEQA: *dest = (MELD_PTR(arg1) != MELD_PTR(arg2)); break;
        case OP_EQA: *dest = (MELD_PTR(arg1) == MELD_PTR(arg2)); break;
      }

		} else if (MOVE(pc)) {
			pcounter old_pc = pc+2;

#ifdef DEBUG_INSTRS
			{
				char src = MOVE_SRC(pc);
				char dst = MOVE_DST(pc);

        printf("MOVE ");
        if(VAL_IS_TUPLE(src))
          printf("tuple");
        else if(VAL_IS_REG(src))
          printf("reg %d", VAL_REG(src));
        else if(VAL_IS_HOST(src))
          printf("host");
        else if(VAL_IS_FIELD(src))
          printf("FIELD");
        else if(VAL_IS_INT(src))
          printf("INT");
        else if(VAL_IS_FLOAT(src))
          printf("float");
				else if(VAL_IS_REVERSE(src))
					printf("reverse");
        else printf("??");

        printf(" ");

        if(VAL_IS_TUPLE(dst))
          printf("tuple");
        else if(VAL_IS_REG(dst))
          printf("reg %d", VAL_REG(dst));
        else if(VAL_IS_HOST(dst))
          printf("host");
        else if(VAL_IS_FIELD(dst))
          printf("FIELD");
        else if(VAL_IS_INT(dst))
          printf("INT");
        else if(VAL_IS_FLOAT(dst))
          printf("float");
				else if(VAL_IS_REVERSE(dst))
					printf("reverse");
        else printf("??");

				printf("\n");
			}
#endif
			size_t size = 0;

			anything src = eval(MOVE_SRC(pc), &tuple, &old_pc, reg);
			anything dst = eval_dst(MOVE_DST(pc), &old_pc, reg, &size);

			memcpy(dst, src, size);

		} else if (ALLOC(pc)) {       /************* ALLOC **************/
		  
			pcounter old_pc = pc+2;
      tuple_t *dst;
      
#if defined(DEBUG_INSTRS) || defined(DEBUG_ALLOCS)
      {
        tuple_type type = ALLOC_TYPE(pc);
        printf("ALLOC %s\n", tuple_names[type]);
      }
#endif

			dst = eval(ALLOC_DST(pc), &tuple, &old_pc, reg);
			
			*dst = ALLOC_TUPLE(TYPE_SIZE(ALLOC_TYPE(pc)));
      memset(*dst, 0, TYPE_SIZE(ALLOC_TYPE(pc)));
			TUPLE_TYPE(*dst) = ALLOC_TYPE(pc);
    } else if (CALL(pc)) {
		 	Register *dst = &reg[CALL_DST(pc)];
			anything args[CALL_ARGS(pc)];

			assert(CALL_ARGS(pc) <= 5);

#ifdef DEBUG_INSTRS
      printf("CALL %d (%d)\n", CALL_ID(pc), CALL_ARGS(pc));
#endif
        
			int i;
			pcounter old_pc = pc+2;
			for (i = 0; i < CALL_ARGS(pc); i++) {
				unsigned char value = CALL_VAL(old_pc);
				old_pc++;
				args[i] = eval(value, &tuple, &old_pc, reg);
			}

			switch (CALL_ARGS(pc)) {
				default:
					break;
				case 0:
					CALL_FUNC(pc)(dst);
					break;
				case 1:
					CALL_FUNC(pc)(dst, args[0]);
					break;
				case 2:
					CALL_FUNC(pc)(dst, args[0], args[1]);
					break;
				case 3:
					CALL_FUNC(pc)(dst, args[0], args[1], args[2]);
					break;
				case 4:
					CALL_FUNC(pc)(dst, args[0], args[1], args[2], args[3]);
					break;
				case 5:
					CALL_FUNC(pc)(dst, args[0], args[1], args[2], args[3], args[4]);
					break;
			}
		}	
	}
	
	return RET_RET;
}

void
tuple_print(tuple_t tuple, FILE *fp)
{
	tuple_type tuple_type = TUPLE_TYPE(tuple);
	int j;

	fprintf(fp, "%s(", TYPE_NAME(tuple_type));
	for(j = 0; j < TYPE_NOARGS(tuple_type); ++j) {
		field_t field = GET_TUPLE_FIELD(tuple, j);

		if (j > 0)
			fprintf(fp, ", ");

		switch(TYPE_ARG_TYPE(tuple_type, j)) {
			case FIELD_INT:
				fprintf(fp, "%d", MELD_INT(field));
				break;
			case FIELD_FLOAT:
				fprintf(fp, "%f", (double)MELD_FLOAT(field));
				break;
			case FIELD_ADDR:
				fprintf(fp, "%p", MELD_PTR(field));
				break;
			case FIELD_LIST_INT:
				fprintf(fp, "list_int[%d][%p]", list_total(MELD_LIST(field)),
						MELD_LIST(field));
				break;
			case FIELD_LIST_FLOAT:
				fprintf(fp, "list_float[%d][%p]", list_total(MELD_LIST(field)),
						MELD_LIST(field));
				break;
			case FIELD_LIST_ADDR: {
        List *list = MELD_LIST(field);
        
        fprintf(fp, "list_addr[%d][%p]", list == NULL ? -1 : list_total(list),
          list);
			  }
				break;
			case FIELD_SET_INT:
				fprintf(fp, "set_int[%d][%p]", set_total(MELD_SET(field)),
						MELD_SET(field));
				break;
			case FIELD_SET_FLOAT:
				fprintf(fp, "set_float[%d][%p]", set_total(MELD_SET(field)),
						MELD_SET(field));
				break;
			case FIELD_TYPE:
        fprintf(fp, "%s", TYPE_NAME(MELD_INT(field)));
        break;
			default:
				assert(0);
				break;
		}
	}
	fprintf(fp, ")");
}

void
tuple_dump(tuple_t tuple)
{
	tuple_print(tuple, stderr);
	fprintf(stderr, "\n");
}

void facts_dump(void)
{
	tuple_type i;

	for (i = 0; i < NUM_TYPES; i++) {
		/* don't print artificial tuple types */
		if (tuple_names[i][0] == '_')
			continue;

		fprintf(stderr, "tuple %s (type %d)\n", tuple_names[i], i);

		if(TYPE_IS_PERSISTENT(i)) {
			persistent_set *persistent = &PERSISTENT[type];
			int size = TYPE_SIZE(i);

			int j;
			for(j = 0; j < persistent->current; ++j) {
				tuple_t tuple = persistent->array + j * size;

				fprintf(stderr, " ");
				tuple_print(tuple, stderr);
				fprintf(stderr, "\n");
			}
		} else {
			tuple_entry *entry = TUPLES[i].head;
			for (; entry != NULL; entry = entry->next) {
				fprintf(stderr, "  ");
				tuple_print(entry->tuple, stderr);
				fprintf(stderr, "x%d\n", entry->records.count);
			}
		}
	}
}

void
print_program_info(void)
{
  /* print program info */
	tuple_type i;
	for(i = 0; i < NUM_TYPES; ++i) {
    printf("TUPLE (%s:%d)\t", tuple_names[i], TYPE_SIZE(i));
    
    printf("[");

    if(TYPE_IS_AGG(i))
      printf("aggregate, ");

    if(TYPE_IS_PERSISTENT(i))
      printf("persistent, ");
    if(TYPE_IS_LINEAR(i))
      printf("linear, ");
		if(TYPE_IS_ROUTING(i))
			printf("routing, ");
		if(TYPE_IS_PROVED(i))
      printf("proved, ");
    
		if(TYPE_NODELTAS(i) > 0)
			printf("deltas:%d, ", TYPE_NODELTAS(i));
    printf("off:%d]\t{", TYPE_OFFSET(i));

		aggregate_t type_aggregate = TYPE_AGGREGATE(i);
		aggregate_field field_aggregate = AGG_FIELD(type_aggregate);
		
		int j;
		for (j = 0; j < TYPE_NOARGS(i); ++j) {
			printf("(");

			if(TYPE_IS_AGG(i) && field_aggregate == j) {
				switch(AGG_AGG(type_aggregate)) {
					case AGG_FIRST:
						printf("first"); break;
					case AGG_MIN_FLOAT:
					case AGG_MIN_INT:
						printf("min"); break;
					case AGG_MAX_FLOAT:
					case AGG_MAX_INT:
						printf("max"); break;
					case AGG_SUM_FLOAT:
					case AGG_SUM_INT:
					case AGG_SUM_LIST_INT:
					case AGG_SUM_LIST_FLOAT:
						printf("sum"); break;
					case AGG_SET_UNION_INT:
					case AGG_SET_UNION_FLOAT:
						printf("set_union"); break;
				}
				printf(" ");
			}

			switch(TYPE_ARG_TYPE(i, j)) {
				case FIELD_INT: printf("int"); break;
				case FIELD_TYPE: printf("type"); break;
				case FIELD_FLOAT: printf("float"); break;
				case FIELD_ADDR: printf("addr"); break;
				case FIELD_LIST_INT: printf("int list"); break;
				case FIELD_LIST_FLOAT: printf("float list"); break;
				case FIELD_LIST_ADDR: printf("addr list"); break;
				case FIELD_SET_INT: printf("int set"); break;
				case FIELD_SET_FLOAT: printf("float set"); break;
			}

			printf(":%d:%d)", TYPE_ARG_OFFSET(i, j), TYPE_ARG_SIZE(i, j));
			
			if(j != TYPE_NOARGS(i)-1)
				printf(",");
		}

		printf("}\n");
	}
}

void
print_program_code(void)
{
	tuple_type i;

	for(i = 0; i < NUM_TYPES; ++i) {
		printf("PROCESS %s:\n", TYPE_NAME(i));

		pcounter pc = TYPE_START(i);
		pcounter next;

		if(i == NUM_TYPES-1) {
			next = meld_prog + size_meld_prog;
		} else {
			next = TYPE_START(i+1);
		}

		for (; pc < next; pc = advance(pc)) {
			if (RETURN(pc)) {
				printf("RETURN\n");
			} else if(IF(pc)) {
				printf("IF (%d) THEN\n", IF_REG(pc));
			} else if(MOVE(pc)) {
				char src = MOVE_SRC(pc);
				char dst = MOVE_DST(pc);
				pcounter m = pc + 2;

				printf("MOVE ");

        if(VAL_IS_TUPLE(src))
          printf("tuple");
        else if(VAL_IS_REG(src))
          printf("reg %d", VAL_REG(src));
        else if(VAL_IS_HOST(src))
          printf("host");
        else if(VAL_IS_FIELD(src)) {
					printf("%d.%d", VAL_FIELD_REG(m), VAL_FIELD_NUM(m));
					m = m + 2;
				} else if(VAL_IS_INT(src)) {
          printf("%d", *(int *)m);
					m += sizeof(int);
				} else if(VAL_IS_FLOAT(src)) {
					printf("%f", *(float *)m);
					m += sizeof(float);
				}

        printf(" TO ");

        if(VAL_IS_REG(dst))
          printf("reg %d", VAL_REG(dst));
        else if(VAL_IS_FIELD(dst))
					printf("%d.%d", VAL_FIELD_REG(m), VAL_FIELD_NUM(m));

				printf("\n");
			} else if(ALLOC(pc)) {
				tuple_type type = ALLOC_TYPE(pc);
				printf("ALLOC %s TO ", TYPE_NAME(type));

				char dst = ALLOC_DST(pc);
				pcounter a = pc + 2;

				if(VAL_IS_REG(dst))
					printf("reg %d", VAL_REG(dst));
				else if(VAL_IS_FIELD(dst))
					printf("%d.%d", VAL_FIELD_REG(a), VAL_FIELD_NUM(a));

				printf("\n");
			} else if(OP(pc)) {
				printf("SET ");
				pcounter m = pc + 3;

				printf("reg %d", OP_DST(pc));

				printf(" TO ");

				char arg1 = OP_ARG1(pc);
				char arg2 = OP_ARG2(pc);

        if(VAL_IS_TUPLE(arg1))
          printf("tuple");
        else if(VAL_IS_REG(arg1))
          printf("reg %d", VAL_REG(arg1));
        else if(VAL_IS_HOST(arg1))
          printf("host");
        else if(VAL_IS_FIELD(arg1)) {
					printf("%d.%d", VAL_FIELD_REG(m), VAL_FIELD_NUM(m));
					m += 2;
				} else if(VAL_IS_INT(arg1)) {
          printf("%d", *(int *)m);
					m += sizeof(int);
				} else if(VAL_IS_FLOAT(arg1)) {
					printf("%f", *(float *)m);
					m += sizeof(float);
				}

				printf(" ");

				switch(OP_OP(pc)) {
					case OP_NEQI: printf("INT NOT EQUAL"); break;
					case OP_EQI: printf("INT EQUAL"); break;
					case OP_LESSI: printf("INT LESSER"); break;
					case OP_LESSEQI: printf("INT LESSER EQUAL"); break;
					case OP_GREATERI: printf("INT GREATER"); break;
					case OP_GREATEREQI: printf("INT GREATER EQUAL"); break;
					case OP_MODI: printf("INT MOD"); break;
					case OP_PLUSI: printf("INT PLUS"); break;
					case OP_MINUSI: printf("INT MINUS"); break;
					case OP_TIMESI: printf("INT TIMES"); break;
					case OP_DIVI: printf("INT DIV"); break;
					case OP_NEQF: printf("FLOAT NOT EQUAL"); break;
					case OP_EQF: printf("FLOAT EQUAL"); break;
					case OP_LESSF: printf("FLOAT LESSER"); break;
					case OP_LESSEQF: printf("FLOAT LESSER EQUAL"); break;
					case OP_GREATERF: printf("FLOAT GREATER"); break;
					case OP_GREATEREQF: printf("FLOAT GREATER EQUAL"); break;
					case OP_MODF: printf("FLOAT MOD"); break;
					case OP_PLUSF: printf("FLOAT PLUS"); break;
					case OP_MINUSF: printf("FLOAT MINUS"); break;
					case OP_TIMESF: printf("FLOAT TIMES"); break;
					case OP_DIVF: printf("FLOAT DIV"); break;
					case OP_NEQA: printf("ADDR NOT EQUAL"); break;
					case OP_EQA: printf("ADDR EQUAL"); break;
				}

				printf(" ");

        if(VAL_IS_TUPLE(arg2))
          printf("tuple");
        else if(VAL_IS_REG(arg2))
          printf("reg %d", VAL_REG(arg2));
        else if(VAL_IS_HOST(arg2))
          printf("host");
        else if(VAL_IS_FIELD(arg2)) {
					printf("%d.%d", VAL_FIELD_REG(m), VAL_FIELD_NUM(m));
					m += 2;
				} else if(VAL_IS_INT(arg2)) {
          printf("%d", *(int *)m);
					m += sizeof(int);
				} else if(VAL_IS_FLOAT(arg2)) {
					printf("%f", *(float *)m);
					m += sizeof(float);
				}

				printf("\n");
			
			} else if(SEND(pc)) {
				char msg = SEND_MSG(pc);
				char rt = SEND_RT(pc);

				printf("SEND reg %d TO reg %d IN %dms\n", VAL_REG(msg), VAL_REG(rt), SEND_DELAY(pc + 3));
			} else if(ITER(pc)) {
				printf("ITERATE OVER %s MATCHING", TYPE_NAME(ITER_TYPE(pc)));

				pcounter m = pc + 4;

				if(!ITER_MATCH_NONE(m)) {
					while(1) {
						pcounter start = m;
					  char fieldnum = ITER_MATCH_FIELD(m);
						char v = ITER_MATCH_VAL(m);
						m += 2;

						printf("\n");
						printf("  (match).%d=", fieldnum);

						if(VAL_IS_REG(v))
							printf("reg %d", VAL_REG(v));
						else if(VAL_IS_HOST(v))
							printf("host");
						else if(VAL_IS_FIELD(v)) {
							printf("%d.%d", VAL_FIELD_REG(m), VAL_FIELD_NUM(m));
							m += 2;
						} else if(VAL_IS_INT(v)) {
							printf("%d", *(int *)m);
							m += sizeof(int);
						} else if(VAL_IS_FLOAT(v)) {
							printf("%f", *(float *)m);
							m += sizeof(float);
						}

						if(ITER_MATCH_END(start))
							break;
					}
				}

				printf("\n");
			} else if(NEXT(pc)) {
				printf("NEXT\n");
			} else if(CALL(pc)) {
				int args = CALL_ARGS(pc);

				printf("CALL func(%d):%d TO %d = (", CALL_ID(pc), args, CALL_DST(pc));

				int i;
				pcounter m = pc + 2;
				for(i = 0; i < args; ++i) {
					char v = CALL_VAL(m);
					++m;

					if(VAL_IS_TUPLE(v))
						printf("tuple");
					else if(VAL_IS_REG(v))
						printf("reg %d", VAL_REG(v));
					else if(VAL_IS_HOST(v))
						printf("host");
					else if(VAL_IS_FIELD(v)) {
						printf("%d.%d", VAL_FIELD_REG(m), VAL_FIELD_NUM(m));
						m += 2;
					} else if(VAL_IS_INT(v)) {
						printf("%d", *(int *)m);
						m += sizeof(int);
					} else if(VAL_IS_FLOAT(v)) {
						printf("%f", *(float *)m);
						m += sizeof(float);
					}

					if(i < args - 1)
						printf(", ");
				}

				printf(")\n");
			} else {
				printf("OTHER\n");
			}
		}

		printf("\n");
	}
}

bool
type_has_code(tuple_type type)
{
	return !RETURN(TYPE_START(type));
}
