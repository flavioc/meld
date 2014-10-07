
#ifndef DB_TUPLE_HPP
#define DB_TUPLE_HPP

#include <vector>
#include <ostream>
#include <list>

#include "vm/defs.hpp"
#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "mem/allocator.hpp"
#include "mem/base.hpp"
#include "utils/types.hpp"

namespace db
{
   
class simple_tuple: public mem::base
{
private:
   vm::predicate *pred;
   vm::tuple *data;
   vm::derivation_count count;
   vm::depth_t depth;
   // if tuple is a final aggregate
   bool is_final_aggregate;

public:

   inline vm::tuple* get_tuple(void) const { return data; }
   
   inline vm::strat_level get_strat_level(void) const { return pred->get_strat_level(); }

	inline vm::predicate* get_predicate(void) const { return pred; }
   
   inline vm::predicate_id get_predicate_id(void) const { return pred->get_id(); }

   inline bool is_aggregate(void) const { return is_final_aggregate; }
   inline void set_as_aggregate(void) { is_final_aggregate = true; }

   void print(std::ostream&) const;

   inline vm::derivation_count get_count(void) const { return count; }
   inline bool reached_zero(void) const { return get_count() == 0; }
   inline void inc_count(const vm::derivation_count& inc) { assert(inc > 0); count += inc; }
   inline void dec_count(const vm::derivation_count& inc) { assert(inc > 0); count -= inc; }
   inline void add_count(const vm::derivation_count& inc) { count += inc; }

   inline vm::depth_t get_depth(void) const { return depth; }
   
   inline size_t storage_size(void) const
   {
      return sizeof(vm::derivation_count) + sizeof(vm::depth_t) + data->get_storage_size(pred);
   }
   
   void pack(utils::byte *, const size_t, int *) const;
   
   static simple_tuple* unpack(vm::predicate *, utils::byte *, const size_t, int *, vm::program *);

   static simple_tuple* create_new(vm::tuple *tuple, vm::predicate *pred, const vm::depth_t depth) { return new simple_tuple(tuple, pred, 1, depth); }

   static simple_tuple* remove_new(vm::tuple *tuple, vm::predicate *pred, const vm::depth_t depth) { return new simple_tuple(tuple, pred, -1, depth); }
   
   static void wipeout(simple_tuple *stpl) { vm::tuple::destroy(stpl->get_tuple(), stpl->get_predicate()); delete stpl; }

   explicit simple_tuple(vm::tuple *_tuple, vm::predicate *_pred, const vm::derivation_count _count, const vm::depth_t _depth = 0):
      pred(_pred), data(_tuple), count(_count), depth(_depth),
      is_final_aggregate(false)
   {}

   explicit simple_tuple(void): // for serialization purposes
      is_final_aggregate(false)
   {
   }

   ~simple_tuple(void);
};

std::ostream& operator<<(std::ostream&, const simple_tuple&);

typedef std::list<simple_tuple*, mem::allocator<simple_tuple*> > simple_tuple_list;
typedef std::vector<simple_tuple*, mem::allocator<simple_tuple*> > simple_tuple_vector;

}

#endif
