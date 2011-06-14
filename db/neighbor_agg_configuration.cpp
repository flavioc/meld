
#include "db/neighbor_agg_configuration.hpp"

using namespace vm;

namespace db
{
   
void
neighbor_agg_configuration::add_to_set(vm::tuple *tuple, const vm::ref_count count)
{
   const predicate *pred(tuple->get_predicate());
   const size_t last_field(pred->num_fields()-1);
   
   const node_val node(tuple->get_node(last_field));
   
   edge_set::iterator it(sent.find(node));
   
   if(it == sent.end()) {
      agg_configuration::add_to_set(tuple, count);
      sent.insert(node);
   } else {
   }
}

bool
neighbor_agg_configuration::all_present(const edge_set& edges) const
{
   for(edge_set::const_iterator it(edges.begin()); it != edges.end(); ++it) {
      const node_val node(*it);
      
      if(sent.find(node) == sent.end())
         return false;
   }
   
   return true;
}

}
