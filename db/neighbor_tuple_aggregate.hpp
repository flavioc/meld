
#ifndef DB_NEIGHBOR_TUPLE_AGGREGATE_HPP
#define DB_NEIGHBOR_TUPLE_AGGREGATE_HPP

#include "db/tuple_aggregate.hpp"
#include "db/edge_set.hpp"

namespace db
{

class neighbor_tuple_aggregate: public tuple_aggregate
{
private:
   
   edge_set neighbors;
   
protected:
   
   virtual agg_configuration *create_configuration(void) const;
   
public:
   
   MEM_METHODS(neighbor_tuple_aggregate)

   explicit neighbor_tuple_aggregate(const vm::predicate *_pred, const edge_set& _neis):
      tuple_aggregate(_pred), neighbors(_neis) {}
};

}

#endif
