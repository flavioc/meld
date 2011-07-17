
#ifndef DB_TUPLE_AGGREGATE_HPP
#define DB_TUPLE_AGGREGATE_HPP

#include <ostream>

#include "mem/base.hpp"
#include "vm/defs.hpp"
#include "db/tuple.hpp"
#include "db/trie.hpp"
#include "db/agg_configuration.hpp"

namespace db
{
   
class tuple_aggregate: public mem::base<tuple_aggregate>
{
private:

   const vm::predicate *pred;
   agg_trie vals;

public:

   void print(std::ostream&) const;

   simple_tuple_list generate(void);

   agg_configuration* add_to_set(vm::tuple *, const vm::ref_count);
   
   bool no_changes(void) const;
   inline bool empty(void) const { return vals.empty(); }
   
   void delete_by_index(const vm::match&);

   explicit tuple_aggregate(const vm::predicate *_pred): pred(_pred) {}

   ~tuple_aggregate(void);
};

std::ostream& operator<<(std::ostream&, const tuple_aggregate&);
   
}

#endif

