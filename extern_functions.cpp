#include "config.h"
#include "extern_functions.h"
#include "node.h"
#include "set_runtime.h"
#include "list_runtime.h"
#include "utils.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

MELD_FUNCTION1(to_float)
{
	MELD_RETURN_FLOAT((meld_float)MELD_INT(MELD_ARG1));
}

MELD_FUNCTION1(float_abs)
{
	MELD_RETURN_FLOAT(fabs(MELD_FLOAT(MELD_ARG1)));
}

MELD_FUNCTION2(print_float)
{
	Node *node = MELD_NODE(MELD_ARG1);
	meld_float fl = MELD_FLOAT(MELD_ARG2);

	printf("Float: " NODE_FORMAT " %f\n", node->order, fl);

	MELD_RETURN_NOTHING();
}

MELD_FUNCTION1(nodeID)
{
	MELD_RETURN_INT((meld_int)MELD_NODE(MELD_ARG1)->order);
}

MELD_FUNCTION2(print_int)
{
	Node *node = MELD_NODE(MELD_ARG1);
	meld_int int_val = MELD_INT(MELD_ARG2);

	printf("Int: " NODE_FORMAT " %d\n", node->order, int_val);

	MELD_RETURN_NOTHING();
}

MELD_FUNCTION0(list_node_new)
{
	List *list = list_node_create();

	MELD_RETURN_LIST(list);
}

MELD_FUNCTION1(list_size)
{
	MELD_RETURN_INT(list_total(MELD_LIST(MELD_ARG1)));
}

MELD_FUNCTION2(list_push_tail_node)
{
	List *clone = list_copy(MELD_LIST(MELD_ARG1));
	
	list_node_push_tail(clone, MELD_NODE(MELD_ARG2));

	MELD_RETURN_LIST(clone);
}

MELD_FUNCTION2(list_push_head_node)
{
	List *clone = list_copy(MELD_LIST(MELD_ARG1));
	
	list_node_push_head(clone, MELD_NODE(MELD_ARG2));

	MELD_RETURN_LIST(clone);
}

MELD_FUNCTION1(generate_random)
{
	MELD_RETURN_INT((meld_int)random_int(MELD_INT(MELD_ARG1)));
}

MELD_FUNCTION0(empty_set)
{
	MELD_RETURN_SET(set_int_create());
}

MELD_FUNCTION2(set_insert_int)
{
	Set *clone = set_copy(MELD_SET(MELD_ARG1));

	set_int_insert(clone, MELD_INT(MELD_ARG2));

	MELD_RETURN_SET(clone);
}

#ifdef BELIEF_PROPAGATION

MELD_FUNCTION1(calculateListSum)
{
	List* list = MELD_LIST(MELD_ARG1);
	list_iterator it = list_get_iterator(list);
	
	if(list_is_float(list)) {
		meld_float sum = 0.0;

		while (list_iterator_has_next(it)) {
			sum += list_iterator_float(it);
			it = list_iterator_next(it);
		}

		MELD_RETURN_FLOAT(sum);
	} else if (list_is_int(list)) {
		meld_int sum = 0;

		while (list_iterator_has_next(it)) {
			sum += list_iterator_int(it);
			it = list_iterator_next(it);
		}

		MELD_RETURN_INT(sum);
	}

	assert(0);
}

MELD_FUNCTION2(sumLists)
{
	List *list1 = MELD_LIST(MELD_ARG1);
	List *list2 = MELD_LIST(MELD_ARG2);

	if(list_total(list1) != list_total(list2)) {
		fprintf(stderr, "sumLists: lists differ in size: %d vs %d\n",
				list_total(list1), list_total(list2));
		exit(EXIT_FAILURE);
	}

	list_iterator it1 = list_get_iterator(list1);
	list_iterator it2 = list_get_iterator(list2);
	List *ret = NULL;

	if(list_is_float(list1)) {
		ret = list_float_create();

		while (list_iterator_has_next(it1)) {
			list_float_push_tail(ret, list_iterator_float(it1) + list_iterator_float(it2));
			it1 = list_iterator_next(it1);
			it2 = list_iterator_next(it2);
		}
	} else {
		ret = list_int_create();

		while (list_iterator_has_next(it1)) {
			list_int_push_tail(ret, list_iterator_int(it1) + list_iterator_int(it2));
			it1 = list_iterator_next(it1);
			it2 = list_iterator_next(it2);
		}
	}

	MELD_RETURN_LIST(ret);
}

MELD_FUNCTION2(minusLists)
{
	List *list1 = MELD_LIST(MELD_ARG1);
	List *list2 = MELD_LIST(MELD_ARG2);

	if(list_total(list1) != list_total(list2)) {
		fprintf(stderr, "minusLists: lists differ in size: %d vs %d\n",
				list_total(list1), list_total(list2));
		exit(EXIT_FAILURE);
	}

	list_iterator it1 = list_get_iterator(list1);
	list_iterator it2 = list_get_iterator(list2);
	List *ret = NULL;

	if(list_is_float(list1)) {
		ret = list_float_create();

		while (list_iterator_has_next(it1)) {
			list_float_push_tail(ret, list_iterator_float(it1) - list_iterator_float(it2));
			it1 = list_iterator_next(it1);
			it2 = list_iterator_next(it2);
		}
	} else {
		ret = list_int_create();

		while (list_iterator_has_next(it1)) {
			list_int_push_tail(ret, list_iterator_int(it1) - list_iterator_int(it2));
			it1 = list_iterator_next(it1);
			it2 = list_iterator_next(it2);
		}
	}

	MELD_RETURN_LIST(ret);
}

extern void normalize_vector(meld_float*, int);

extern int colors;

MELD_FUNCTION1(normalizeList)
{
	List *list = MELD_LIST(MELD_ARG1);

	int total = list_total(list);
	list_iterator it = list_get_iterator(list);
	
	if(list_is_float(list)) {
		meld_float vec[total];
		int i = 0;

		while (list_iterator_has_next(it)) {
			vec[i++] = list_iterator_float(it);
			it = list_iterator_next(it);
		}

		normalize_vector(vec, total);

		it = list_get_iterator(list);
		i = 0;
		while (list_iterator_has_next(it)) {
			list_iterator_float(it) = vec[i++];
			it = list_iterator_next(it);
		}

		MELD_RETURN_LIST(list);
	} else {
		fprintf(stderr, "normalizeList for ints not supported!");
		assert(0);
		exit(EXIT_FAILURE);
	}
}

extern meld_float damping;
extern int colors;
extern meld_float *edge_potential;

MELD_FUNCTION1(convolve)
{
	meld_float vec[colors];
	int i;
	for(i = 0; i < colors; ++i)
		vec[i] = 0.0;

	List *cavity = MELD_LIST(MELD_ARG1);
	List *ret = list_float_create();

	int x, y;
	for(x = 0; x < colors; ++x) {
		meld_float sum = 0.0;
		list_iterator cav = list_get_iterator(cavity);

		for(y = 0; y < colors; ++y, cav = list_iterator_next(cav))
			sum += exp(edge_potential[x + y * colors] + list_iterator_float(cav));

		if(sum == 0) sum = 1.17549e-38;

		list_float_push_tail(ret, log(sum));
	}

	MELD_RETURN_LIST(ret);
}

MELD_FUNCTION2(damp)
{
	List *conv = MELD_LIST(MELD_ARG1);
	List *out = MELD_LIST(MELD_ARG2);

	if(damping != 0.0) {
		list_iterator it_conv = list_get_iterator(conv);
		list_iterator it_out = list_get_iterator(out);

		for(; list_iterator_has_next(it_conv);
				it_conv = list_iterator_next(it_conv),
				it_out = list_iterator_next(it_out))
		{
			list_iterator_float(it_conv) = log(damping * exp(list_iterator_float(it_out))
					+ (1.0 - damping) * exp(list_iterator_float(it_conv)));
		}
	}

	MELD_RETURN_LIST(conv);
}

#endif /* BELIEF_PROPAGATION */

