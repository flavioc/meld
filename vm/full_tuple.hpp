
#ifndef VM_FULL_TUPLE_HPP
#define VM_FULL_TUPLE_HPP

#include <vector>
#include <ostream>
#include <list>

#include "vm/defs.hpp"
#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "mem/allocator.hpp"
#include "mem/base.hpp"
#include "utils/types.hpp"

namespace vm
{
   
struct full_tuple: public mem::base
{
public:
   vm::tuple *data;

private:
   vm::predicate *pred;
   vm::derivation_count count;
   vm::depth_t depth;

public:

   inline vm::tuple* get_tuple(void) const { return data; }
   
   inline vm::strat_level get_strat_level(void) const { return get_predicate()->get_strat_level(); }

	inline vm::predicate* get_predicate(void) const { return (vm::predicate*)((intptr_t)pred & (~(intptr_t)0x1)); }
   
   inline bool is_aggregate(void) const { return (intptr_t)pred & 0x1; }
   inline void set_as_aggregate(void) { pred = (vm::predicate*)(0x1 | (intptr_t)pred); }

   void print(std::ostream&) const;

   inline vm::derivation_count get_count(void) const { return count; }
   inline bool reached_zero(void) const { return get_count() == 0; }
   inline void inc_count(const vm::derivation_count& inc) { assert(inc > 0); count += inc; }
   inline void dec_count(const vm::derivation_count& inc) { assert(inc > 0); count -= inc; }
   inline void add_count(const vm::derivation_count& inc) { count += inc; }

   inline vm::depth_t get_depth(void) const { return depth; }
   
   inline size_t storage_size(void) const
   {
      return sizeof(vm::derivation_count) + sizeof(vm::depth_t) + data->get_storage_size(get_predicate());
   }
   
   void pack(utils::byte *, const size_t, int *) const;
   
   static full_tuple* unpack(vm::predicate *, utils::byte *, const size_t, int *, vm::program *);

   static full_tuple* create_new(vm::tuple *tuple, vm::predicate *pred, const vm::depth_t depth) { return new full_tuple(tuple, pred, 1, depth); }

   static full_tuple* remove_new(vm::tuple *tuple, vm::predicate *pred, const vm::depth_t depth) { return new full_tuple(tuple, pred, -1, depth); }
   
#ifdef GC_NODES
   static void wipeout(full_tuple *stpl, candidate_gc_nodes& gc_nodes) {
#else
   static void wipeout(full_tuple *stpl) {
#endif
      vm::tuple::destroy(stpl->get_tuple(), stpl->get_predicate()
#ifdef GC_NODES
            , gc_nodes
#endif
      );
      delete stpl;
   }

   explicit full_tuple(vm::tuple *_tuple, vm::predicate *_pred, const vm::derivation_count _count, const vm::depth_t _depth = 0):
      data(_tuple), pred(_pred), count(_count), depth(_depth)
   {}

   explicit full_tuple(void) // for serialization purposes
   {
   }

   ~full_tuple(void);
};

std::ostream& operator<<(std::ostream&, const full_tuple&);

typedef utils::intrusive_list<full_tuple, utils::indirect_intrusive_next<full_tuple>,
        utils::indirect_intrusive_prev<full_tuple> > full_tuple_list;
typedef std::vector<full_tuple, mem::allocator<full_tuple>> tuple_array;

}

#endif
