
#include "db/neighbor_agg_configuration.hpp"

using namespace vm;

namespace db
{
   
void
neighbor_agg_configuration::add_to_set(vm::tuple *tuple, const vm::ref_count count)
{
   const size_t old_size(size());
   
   const predicate *pred(tuple->get_predicate());
   const size_t last_field(pred->num_fields()-1);
   
   const node_val node(tuple->get_node(last_field));
   
#ifdef USE_OLD_NEIGHBOR_CHECK
   edge_set::iterator it(sent.find(node));
   
   if(it == sent.end()) {
      agg_configuration::add_to_set(tuple, count);
      
      sent.insert(node);
      
      assert(size() > old_size);
   }
#else
   edge_set::iterator it(target.find(node));
   
   if(it != target.end()) {
      agg_configuration::add_to_set(tuple, count);
      
      target.erase(it);
      
      assert(size() > old_size);
   }
#endif
}

bool
neighbor_agg_configuration::all_present(void) const
{
#ifdef USE_OLD_NEIGHBOR_CHECK
   for(edge_set::const_iterator it(target.begin()); it != target.end(); ++it) {
      const node_val node(*it);
      
      if(sent.find(node) == sent.end())
         return false;
   }
   
   return true;
#else
   return target.empty();
#endif
}

vm::tuple*
neighbor_agg_configuration::do_generate(const aggregate_type typ, const field_num num)
{
   vm::tuple *ret(agg_configuration::do_generate(typ, num));
   
   if(ret) {
      const predicate *pred(ret->get_predicate());
      const size_t last_field(pred->num_fields()-1);
      
      ret->set_node(last_field, 0);
   }
   
   return ret;
}

}
