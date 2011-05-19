
#ifndef DB_TUPLE_AGGREGATE_HPP
#define DB_TUPLE_AGGREGATE_HPP

#include <ostream>

#include "mem/base.hpp"
#include "vm/defs.hpp"
#include "db/tuple.hpp"
#include "db/agg_configuration.hpp"

namespace db
{
   
class tuple_aggregate: public mem::base<tuple_aggregate>
{
private:

   typedef std::list<agg_configuration*, mem::allocator<agg_configuration*> > agg_conf_list;

   const vm::predicate *pred;
   agg_conf_list values;

public:

   void print(std::ostream&) const;

   simple_tuple_list generate(void);

   agg_configuration* add_to_set(vm::tuple *, const vm::ref_count);
   
   const bool no_changes(void) const;
   inline const bool empty(void) const { return values.empty(); }
   
   void delete_by_first_int_arg(const vm::int_val);
   void delete_all(void);

   explicit tuple_aggregate(const vm::predicate *_pred): pred(_pred) {}

   ~tuple_aggregate(void);
};

std::ostream& operator<<(std::ostream&, const tuple_aggregate&);
   
}

#endif

