
#ifndef ALLOCATOR_DEFS_H
#define ALLOCATOR_DEFS_H

#include <pthread.h>

#define GC_FIELD_NEXT _gc_next
#define GC_MARK_MASK 0x01

#define GC_GET_NEXT(Type, ptr) ((Type*)(((unsigned long int)(ptr)->GC_FIELD_NEXT) & ~(GC_MARK_MASK)))
#define GC_SET_NEXT(ptr, val) ((ptr)->GC_FIELD_NEXT = (val))
#define GC_MARK_OBJECT(Type, ptr) ((ptr)->GC_FIELD_NEXT = (Type*)((unsigned long int)((ptr)->GC_FIELD_NEXT) | GC_MARK_MASK))
#define GC_UNMARK_OBJECT(Type, ptr) ((ptr)->GC_FIELD_NEXT = GC_GET_NEXT(Type, ptr))
#define GC_OBJECT_MARKED(ptr) ((unsigned long int)((ptr)->GC_FIELD_NEXT) & GC_MARK_MASK)

#define GC_DATA(Type) 	\
	Type *GC_FIELD_NEXT

#endif /* ALLOCATOR_DEFS_H */
