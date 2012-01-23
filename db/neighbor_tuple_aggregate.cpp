
#include "db/neighbor_tuple_aggregate.hpp"
#include "db/neighbor_agg_configuration.hpp"

namespace db
{
   
agg_configuration*
neighbor_tuple_aggregate::create_configuration(void) const
{
   return new neighbor_agg_configuration(pred, neighbors);
}

}