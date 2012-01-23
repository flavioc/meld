
#ifndef DB_TUPLE_HPP
#define DB_TUPLE_HPP

#include "conf.hpp"

#include <vector>
#include <ostream>
#include <list>

#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif

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
   vm::tuple *data;
   vm::ref_count count;

public:

   MEM_METHODS(simple_tuple)

   inline vm::tuple* get_tuple(void) const { return data; }
   
   inline vm::strat_level get_strat_level(void) const
   {
      return get_tuple()->get_predicate()->get_strat_level();
   }
   
   inline vm::predicate_id get_predicate_id(void) const
   {
      return get_tuple()->get_predicate_id();
   }

   void print(std::ostream&) const;

   inline vm::ref_count get_count(void) const { return count; }
   
   inline bool reached_zero(void) const { return get_count() == 0; }

   inline void inc_count(const vm::ref_count& inc) { assert(inc > 0); count += inc; }

   inline void dec_count(const vm::ref_count& inc) { assert(inc > 0); count -= inc; }
   
   inline void add_count(const vm::ref_count& inc) { count += inc; }
   
#ifdef COMPILE_MPI
   inline size_t storage_size(void) const
   {
      return sizeof(vm::ref_count) + data->get_storage_size();
   }
   
   void pack(utils::byte *, const size_t, int *, MPI_Comm) const;
   
   static simple_tuple* unpack(utils::byte *, const size_t, int *, MPI_Comm);
#endif

   static simple_tuple* create_new(vm::tuple *tuple) { return new simple_tuple(tuple, 1); }

   static simple_tuple* remove_new(vm::tuple *tuple) { return new simple_tuple(tuple, -1); }
   
   static void wipeout(simple_tuple *stpl) { delete stpl->get_tuple(); delete stpl; }

   explicit simple_tuple(vm::tuple *_tuple, const vm::ref_count _count):
      data(_tuple), count(_count)
   {}

   explicit simple_tuple(void) {} // for serialization purposes

   ~simple_tuple(void);
};

std::ostream& operator<<(std::ostream&, const simple_tuple&);

typedef std::list<simple_tuple*, mem::allocator<simple_tuple*> > simple_tuple_list;

}

#endif
