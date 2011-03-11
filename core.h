#ifndef __CORE_H_
#define __CORE_H_

#include <stdlib.h>
#include <stdio.h>

#include "api.h"
#include "model.h"

/* print tuple allocations */
//#define TUPLE_ALLOC_DEBUG 1
/* tuple allocation checks */
#define TUPLE_ALLOC_CHECKS 1

/* macros */
#define IF(x)     (((*(const unsigned char*)(x))&0xe0) == 0x60)
#define ELSE(x)   ((*(const unsigned char*)(x)) == 0x02)
#define ITER(x)   (((*(const unsigned char*)(x))&0xff) == 0xa0)
#define NEXT(x)   ((*(const unsigned char*)(x)) == 0x01)
#define SEND(x)   (((*(const unsigned char*)(x))&0xfc) == 0x08)
#define REMOVE(x) (((*(const unsigned char*)(x))&0xe0) == 0x80)
#define OP(x)     (((*(const unsigned char*)(x))&0xc0) == 0xc0)
#define MOVE(x)   (((*(const unsigned char*)(x))&0xf0) == 0x30)
#define ALLOC(x)  (((*(const unsigned char*)(x))&0xe0) == 0x40)
#define RETURN(x) ((*(const unsigned char*)(x)) == 0x00)
#define CALL(x)   (((*(const unsigned char*)(x)) & 0xf0) == 0x20)

#define IF_REG(x)     ((*(const unsigned char*)(x)+1)&0x1f)
#define IF_JUMP(x)    (*(unsigned short*)((const unsigned char*)(x)+2))
#define IF_BASE       4

#define ITER_TYPE(x)  ((*(const unsigned char*)((x)+1))&0x7f)
#define ITER_JUMP(x)  (*(unsigned short*)((const unsigned char*)((x)+2)))
#define ITER_MATCH_END(x)   (((*(const unsigned char*)((x)+1))&0xc0) == 0x40)
#define ITER_MATCH_NONE(x)  (((*(const unsigned char*)((x)+1))&0xc0) == 0xc0)
#define ITER_MATCH_FIELD(x)   (*(const unsigned char*)(x))
#define ITER_BASE     4

#define ITER_MATCH_VAL(x)   (((*(const unsigned char*)((x)+1))&0x3f))
#define REMOVE_REG(x) ((*(const unsigned char*)(x))&0x1f)

#define SEND_MSG(x)   ((((*(const unsigned char*)(x))&0x3) << 3) | \
                      (((*(const unsigned char*)((x)+1))&0xe0) >> 5))
#define SEND_RT(x)    ((*(const unsigned char*)((x)+1))&0x1f)
#define SEND_DELAY(x) (*(const unsigned char *)((x)+2))
#define SEND_ARG1(x)  ((*((const unsigned char*)(x)+2)) & 0x3f)
#define SEND_BASE 3

#define OP_ARG1(x)    (((*(const unsigned char*)(x)) & 0x3f))
#define OP_ARG2(x)    (((*(const unsigned char*)((x)+1)) & 0xfc) >> 2)
#define OP_OP(x)      ((*(const unsigned char*)((x)+2)) & 0x1f)
#define OP_DST(x)     ((((*(const unsigned char*)((x)+1)) & 0x03) << 3) | \
                      (((*(const unsigned char*)((x)+2)) & 0xe0) >> 5))

#define MOVE_SRC(x)   ((((*(const unsigned char*)(x))&0xf) << 2) | \
					   (((*(const unsigned char*)((x)+1))&0xc0) >> 6))
#define MOVE_DST(x)   (((*(const unsigned char*)((x)+1))&0x3f))

#define ALLOC_TYPE(x) ((((*(const unsigned char *)(x))&0x1f) << 2) | \
					   (((*(const unsigned char *)(x+1))&0xc0) >> 6))
#define ALLOC_DST(x)  ((*(const unsigned char *)((x)+1))&0x3f)

#define CALL_VAL(x)   (*(const unsigned char *)(x))
#define CALL_DST(x)   ((*(const unsigned char *)((x)+1)) & 0x1f)
#define CALL_ID(x)    ((((*(const unsigned char *)((x))) & 0x0f) << 3) | \
						(((*(const unsigned char *)((x)+1)) & 0xe0) >> 5))

#define CALL_ARGS(x)  (extern_functs_args[CALL_ID(x)])
#define CALL_FUNC(x)  (extern_functs[CALL_ID(x)])

#define OP_NEQF       0x0
#define OP_NEQI       0x1
#define OP_EQF        0x2
#define OP_EQI        0x3
#define OP_LESSF      0x4
#define OP_LESSI      0x5
#define OP_LESSEQF    0x6
#define OP_LESSEQI    0x7
#define OP_GREATERF   0x8
#define OP_GREATERI   0x9
#define OP_GREATEREQF 0xa
#define OP_GREATEREQI 0xb
#define OP_MODF       0xc
#define OP_MODI       0xd
#define OP_PLUSF      0xe
#define OP_PLUSI      0xf
#define OP_MINUSF     0x10
#define OP_MINUSI     0x11
#define OP_TIMESF     0x12
#define OP_TIMESI     0x13
#define OP_DIVF       0x14
#define OP_DIVI       0x15
#define OP_NEQA       0x16
#define OP_EQA        0x17

#define VAL_IS_REG(x)   (((const unsigned char)(x)) & 0x20)
#define VAL_IS_TUPLE(x) (((const unsigned char)(x)) == 0x1f)
#define VAL_IS_FLOAT(x) (((const unsigned char)(x)) == 0x00)
#define VAL_IS_INT(x)   (((const unsigned char)(x)) == 0x01)
#define VAL_IS_FIELD(x) (((const unsigned char)(x)) == 0x02)
#define VAL_IS_HOST(x)  (((const unsigned char)(x)) == 0x03)
#define VAL_IS_REVERSE(x) (((const unsigned char)(x)) == 0x04)

#define VAL_REG(x) (((const unsigned char)(x)) & 0x1f)
#define VAL_FIELD_NUM(x) ((*(const unsigned char *)(x)) & 0xff)
#define VAL_FIELD_REG(x) ((*(const unsigned char *)((x)+1)) & 0x1f)

#define TYPE_DESCRIPTOR_SIZE 7
#define DELTA_SIZE 2
#define TYPE_FIELD_TYPE unsigned char
#define TYPE_FIELD_SIZE sizeof(TYPE_FIELD_TYPE)

#define TUPLE_TYPE(x)   (*(TYPE_FIELD_TYPE *)(x))
#define TUPLE_FIELD(x,off)  ((anything)(((unsigned char*)(x)) + TYPE_FIELD_SIZE + (off)))

#define TYPE_OFFSET(x)     (meld_prog[1 + (x)])
#define TYPE_DESCRIPTOR(x) ((unsigned char *)(meld_prog + TYPE_OFFSET(x)))

#define TYPE_PROPERTIES(x) (*(TYPE_DESCRIPTOR(x) + 2))
#define TYPE_AGGREGATE(x)  (*(TYPE_DESCRIPTOR(x) + 3))
#define TYPE_NOARGS(x)     (*(TYPE_DESCRIPTOR(x) + 5))
#define TYPE_STRATIFICATION_ROUND(x) (*(TYPE_DESCRIPTOR(x)+4))
#define TYPE_IS_STRATIFIED(x) (TYPE_STRATIFICATION_ROUND(x) > 0)
#define TYPE_ARGS_DESC(x)  ((unsigned char*)(TYPE_DESCRIPTOR(x)+TYPE_DESCRIPTOR_SIZE))
#define TYPE_NODELTAS(x)   (*(TYPE_DESCRIPTOR(x) + 6))
#define TYPE_DELTAS(x)     (TYPE_ARGS_DESC(x) + 1*TYPE_NOARGS(x))
#define TYPE_START(x)      ((unsigned char*)(meld_prog + *(unsigned short *)TYPE_DESCRIPTOR(x)))
#define TYPE_NAME(x)       (tuple_names[x])
#define TYPE_ARG_DESC(x, f) ((unsigned char *)(TYPE_ARGS_DESC(x)+1*(f)))
#define TYPE_ARG_TYPE(x, f) ((unsigned char)(*TYPE_ARG_DESC(x, f)))

#define TYPE_SIZE(x)       (arguments[(x) * 2 + 1])
#define TYPE_ARGS(x)       (arguments + arguments[(x) * 2])

#define TYPE_ARG(x, f)     (TYPE_ARGS(x)+2*(f))
#define TYPE_ARG_SIZE(x, f) (*TYPE_ARG(x, f))
#define TYPE_ARG_OFFSET(x, f)   (*(TYPE_ARG(x, f) + 1))

#define SET_TUPLE_FIELD(tuple, field, data) \
		memcpy(TUPLE_FIELD(tuple, TYPE_ARG_OFFSET(TUPLE_TYPE(tuple), field)), \
				data, TYPE_ARG_SIZE(TUPLE_TYPE(tuple), field))
#define GET_TUPLE_FIELD(tuple, field) \
		TUPLE_FIELD(tuple, TYPE_ARG_OFFSET(TUPLE_TYPE(tuple), field))
#define GET_TUPLE_SIZE(tuple, field) \
		TYPE_ARG_SIZE(TUPLE_TYPE(tuple), field)
		
#define TYPE_IS_AGG(x)        (TYPE_PROPERTIES(x) & 0x01)
#define TYPE_IS_PERSISTENT(x) (TYPE_PROPERTIES(x) & 0x02)
#define TYPE_IS_LINEAR(x) 		(TYPE_PROPERTIES(x) & 0x04)
#define TYPE_IS_REGULAR(x)    (!TYPE_IS_LINEAR(x))
#define TYPE_IS_DELETE(x) 		(TYPE_PROPERTIES(x) & 0x08)
#define TYPE_IS_SCHEDULE(x) 	(TYPE_PROPERTIES(x) & 0x10)
#define TYPE_IS_ROUTING(x) 		(TYPE_PROPERTIES(x) & 0x20)
#define TYPE_IS_PROVED(x)     (TYPE_PROPERTIES(x) & 0x40)
#define TYPE_IS_NOTHING(x)		(TYPE_PROPERTIES(x) == 0x00)

#define AGG_AGG(x)    (((x) & (0xf0)) >> 4)
#define AGG_FIELD(x)  ((x) & 0x0f)

#define AGG_NONE 0
#define AGG_FIRST 1
#define AGG_MAX_INT 2
#define AGG_MIN_INT 3
#define AGG_SUM_INT 4
#define AGG_MAX_FLOAT 5
#define AGG_MIN_FLOAT 6
#define AGG_SUM_FLOAT 7
#define AGG_SET_UNION_INT 8
#define AGG_SET_UNION_FLOAT 9
#define AGG_SUM_LIST_INT 10
#define AGG_SUM_LIST_FLOAT 11

#define FIELD_INT 0x0
#define FIELD_FLOAT 0x1
#define FIELD_ADDR 0x2
#define FIELD_OTHER 0x2
#define FIELD_LIST_INT 0x3
#define FIELD_LIST_FLOAT 0x4
#define FIELD_LIST_ADDR 0x5
#define FIELD_SET_INT 0x6
#define FIELD_SET_FLOAT 0x7
#define FIELD_TYPE 0x8

#define TYPE_NEIGHBOR		0
#define TYPE_NEIGHBORCOUNT	1
#define TYPE_VACANT			2
#define TYPE_SETCOLOR		3
#define TYPE_SETCOLOR2		4

#define NUM_TYPES  (meld_prog[0])

#define DELTA_TYPE(ori, id) (*(unsigned char*)(deltas[ori] + (id)*DELTA_SIZE))
#define DELTA_POSITION(ori, id) (*(unsigned char*)(deltas[ori] + (id)*DELTA_SIZE + 1))
#define DELTA_WITH(ori) (delta_sizes[ori])
#define DELTA_TOTAL(ori) (delta_sizes[ori])

#define RET_RET 0
#define RET_NEXT 1

extern const unsigned char meld_prog[];
extern const unsigned int size_meld_prog;
typedef Register (*extern_funct_type)();
extern extern_funct_type extern_functs[];
extern int extern_functs_args[];
extern char *tuple_names[];
extern unsigned char *arguments;
extern int *delta_sizes;

static inline tuple_t
tuple_alloc(tuple_type type)
{
#ifdef TUPLE_ALLOC_CHECKS
  if(type >= NUM_TYPES || type < 0) {
    fprintf(stderr, "Unrecognized type: %d\n", type);
    exit(EXIT_FAILURE);
  }
#endif
  
	tuple_t tuple = ALLOC_TUPLE(TYPE_SIZE(type));

	TUPLE_TYPE(tuple) = type;
	
#ifdef TUPLE_ALLOC_DEBUG
  printf("New %s(%d) tuple\n", tuple_names[type], type);
#endif
#ifdef STATISTICS
  stats_tuples_allocated(type);
#endif

	return tuple;
}

void tuple_handle(tuple_t tuple, ref_count isNew, Register *reg);
void tuple_send(tuple_t tuple, void *rt, meld_int delay, ref_count isNew);
void tuple_do_handle(tuple_type type,	tuple_t tuple, ref_count isNew, Register *reg);
int tuple_process(tuple_t tuple, unsigned char *pc,
	ref_count isNew, Register *reg);
void tuple_print(tuple_t tuple, FILE *fp);
void tuple_dump(tuple_t tuple);

void init_deltas(void);
void init_fields(void);
void facts_dump(void);
void init_consts(void);

tuple_entry* queue_enqueue(tuple_queue *queue, tuple_t tuple, record_type isNew);
bool queue_is_empty(tuple_queue *queue);
tuple_t queue_dequeue(tuple_queue *queue, ref_count *isNew);
tuple_t queue_pop_tuple(tuple_queue *queue);
void queue_push_tuple(tuple_queue *queue, tuple_entry *entry);

static inline void
queue_init(tuple_queue *queue)
{
  queue->head = queue->tail = NULL;
}

static inline bool
p_empty(tuple_pqueue *q)
{
	return q->queue == NULL;
}

static inline tuple_pentry*
p_peek(tuple_pqueue *q)
{
	return q->queue;
}

tuple_pentry *p_dequeue(tuple_pqueue *q);
void p_enqueue(tuple_pqueue *q, meld_int priority, tuple_t tuple,
		void *rt, record_type isNew);

void print_program_info(void);
void print_program_code(void);

bool type_has_code(tuple_type type);

extern tuple_type TYPE_INIT;
extern tuple_type TYPE_EDGE;
extern tuple_type TYPE_COLOCATED;
extern tuple_type TYPE_PROVED;
extern tuple_type TYPE_TERMINATE;
extern tuple_type TYPE_TERMINATED;

#endif

