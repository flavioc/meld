
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
#include "mem/node.hpp"
#include "utils/types.hpp"

namespace vm {

struct full_tuple : public mem::base {
   public:
   vm::tuple *data;

   private:
   vm::predicate *pred;
   derivation_direction dir;
   vm::depth_t depth;

   public:
   inline vm::tuple *get_tuple(void) const { return data; }

   inline vm::strat_level get_strat_level(void) const {
      return get_predicate()->get_strat_level();
   }

   inline vm::predicate *get_predicate(void) const {
      return (vm::predicate *)((intptr_t)pred & (~(intptr_t)0x1));
   }

   inline bool is_aggregate(void) const { return (intptr_t)pred & 0x1; }
   inline void set_as_aggregate(void) {
      pred = (vm::predicate *)(0x1 | (intptr_t)pred);
   }

   void print(std::ostream &) const;

   inline derivation_direction get_dir(void) const { return dir; }

   inline vm::depth_t get_depth(void) const { return depth; }

   inline size_t storage_size(void) const {
      return sizeof(derivation_direction) + sizeof(vm::depth_t) +
             data->get_storage_size(get_predicate());
   }

   void pack(utils::byte *, const size_t, int *) const;

   static full_tuple *unpack(vm::predicate *, utils::byte *, const size_t,
                             int *, vm::program *, mem::node_allocator *alloc);

   static full_tuple *create_new(vm::tuple *tuple, vm::predicate *pred,
                                 const vm::depth_t depth) {
      return new full_tuple(tuple, pred, POSITIVE_DERIVATION, depth);
   }

   static full_tuple *remove_new(vm::tuple *tuple, vm::predicate *pred,
                                 const vm::depth_t depth) {
      return new full_tuple(tuple, pred, NEGATIVE_DERIVATION, depth);
   }

   static void wipeout(full_tuple *stpl, mem::node_allocator *alloc, candidate_gc_nodes &gc_nodes) {
      vm::tuple::destroy(stpl->get_tuple(), stpl->get_predicate(), alloc, gc_nodes);
      delete stpl;
   }

   explicit full_tuple(vm::tuple *_tuple, const vm::predicate *_pred,
                       const derivation_direction _dir,
                       const vm::depth_t _depth = 0)
       : data(_tuple), pred((vm::predicate*)_pred), dir(_dir), depth(_depth) {}

   explicit full_tuple(void)  // for serialization purposes
   {}

   ~full_tuple(void);
};

std::ostream &operator<<(std::ostream &, const full_tuple &);

typedef utils::intrusive_list<
    full_tuple, utils::indirect_intrusive_next<full_tuple>,
    utils::indirect_intrusive_prev<full_tuple>> full_tuple_list;
}

#endif
