
#include "db/tuple_aggregate.hpp"

using namespace vm;
using namespace std;

namespace db
{

//#define DEBUG_AGGS
   
agg_configuration*
tuple_aggregate::create_configuration(void) const
{
   return new agg_configuration();
}
   
agg_configuration*
tuple_aggregate::add_to_set(vm::tuple *tpl, vm::predicate *pred,
      const derivation_direction dir, const depth_t depth,
      mem::node_allocator *alloc, candidate_gc_nodes& gc_nodes)
{
   agg_trie_leaf *leaf(vals.find_configuration(tpl, pred));
   agg_configuration *conf;
   
   if(leaf->get_conf() == nullptr) {
      conf = create_configuration();
      leaf->set_conf(conf);
   } else {
      conf = leaf->get_conf();
   }
   
#ifdef DEBUG_AGGS
   cout << "----> Before:" << endl;
   conf->print(cout);
#endif
   conf->add_to_set(tpl, pred, dir, depth, alloc, gc_nodes);
#ifdef DEBUG_AGGS
   cout << "Add " << *tpl << " " << many << " with depth " << depth << endl;
   conf->print(cout);
#endif
   
   return conf;
}

full_tuple_list
tuple_aggregate::generate(mem::node_allocator *alloc)
{
   const aggregate_type typ(pred->get_aggregate_type());
   const field_num field(pred->get_aggregate_field());
   full_tuple_list ls;
   
   for(agg_trie::iterator it(vals.begin());
      it != vals.end(); )
   {
      agg_configuration *conf(*it);
      
      assert(conf != nullptr);
      
      if(conf->has_changed())
         conf->generate(pred, typ, field, ls, alloc);
      
      assert(!conf->has_changed());
      
      if(conf->is_empty()) {
         candidate_gc_nodes gc_nodes;
         it = vals.erase(it, pred, alloc, gc_nodes);
         assert(gc_nodes.empty());
      } else
         it++;
   }
   
   return ls;
}

bool
tuple_aggregate::no_changes(void) const
{
   for(agg_trie::const_iterator it(vals.begin());
      it != vals.end();
      ++it)
   {
      agg_configuration *conf(*it);
      
      if(conf->has_changed())
         return false;
   }
   
   return true;
}

void
tuple_aggregate::delete_by_index(const match& m, mem::node_allocator *alloc, candidate_gc_nodes& gc_nodes)
{
   vals.delete_by_index(pred, m, alloc, gc_nodes);
}

tuple_aggregate::~tuple_aggregate(void)
{
}

void
tuple_aggregate::wipeout(mem::node_allocator *alloc, candidate_gc_nodes& gc_nodes)
{
   for(agg_trie::const_iterator it(vals.begin());
      it != vals.end();
      it++)
   {
      agg_configuration *conf(*it);
      assert(conf != nullptr);
      conf->wipeout(pred, alloc, gc_nodes);
   }
   vals.wipeout(pred, alloc, gc_nodes);
}

void
tuple_aggregate::print(ostream& cout) const
{
   cout << "agg of " << *pred << ":" << endl;
   
   for(agg_trie::const_iterator it(vals.begin());
      it != vals.end();
      it++)
   {
      agg_configuration *conf(*it);
      assert(conf != nullptr);
      conf->print(cout, pred);
      cout << endl;
   }
}

ostream&
operator<<(ostream& cout, const tuple_aggregate& agg)
{
   agg.print(cout);
   return cout;
}

}
