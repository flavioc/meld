
#ifndef DB_TUPLE_AGGREGATE_HPP
#define DB_TUPLE_AGGREGATE_HPP

#include <ostream>

#include "mem/base.hpp"
#include "vm/defs.hpp"
#include "vm/full_tuple.hpp"
#include "db/trie.hpp"
#include "db/agg_configuration.hpp"

namespace db
{

class tuple_aggregate: public mem::base
{
protected:
   vm::predicate *pred;
   
private:

   agg_trie vals;

protected:
   
   virtual agg_configuration* create_configuration(void) const;
   
public:

   void print(std::ostream&) const;

   vm::full_tuple_list generate(
#ifdef GC_NODES
         vm::candidate_gc_nodes&
#endif
         );

   agg_configuration* add_to_set(vm::tuple *, vm::predicate *, const vm::derivation_count, const vm::depth_t
#ifdef GC_NODES
         , vm::candidate_gc_nodes&
#endif
         );
   
   bool no_changes(void) const;
   inline bool empty(void) const { return vals.empty(); }
   
   void delete_by_index(const vm::match&
#ifdef GC_NODES
         , vm::candidate_gc_nodes&
#endif
         );

   explicit tuple_aggregate(vm::predicate *_pred): pred(_pred) {}

   void wipeout(
#ifdef GC_NODES
         vm::candidate_gc_nodes&
#endif
         );

   ~tuple_aggregate(void);
};

std::ostream& operator<<(std::ostream&, const tuple_aggregate&);
   
}

#endif

