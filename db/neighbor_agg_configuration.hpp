
#ifndef DB_NEIGHBOR_AGG_CONFIGURATION_HPP
#define DB_NEIGHBOR_AGG_CONFIGURATION_HPP

#include "db/edge_set.hpp"
#include "db/agg_configuration.hpp"
#include "mem/base.hpp"

namespace db
{

class neighbor_agg_configuration: public agg_configuration
{
private:
   
#ifdef USE_OLD_NEIGHBOR_CHECK
   edge_set sent;
#endif
   edge_set target;
   
   virtual vm::tuple *do_generate(const vm::aggregate_type, const vm::field_num);
   
public:
   
   MEM_METHODS(neighbor_agg_configuration)
   
   virtual void add_to_set(vm::tuple *, const vm::ref_count);

   bool all_present(void) const;
   
   explicit neighbor_agg_configuration(const vm::predicate *_pred, const edge_set& _target):
      agg_configuration(_pred), target(_target)
   {
   }

   virtual ~neighbor_agg_configuration(void) {}
};

}

#endif
