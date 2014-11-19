
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
tuple_aggregate::add_to_set(vm::tuple *tpl, vm::predicate *pred, const derivation_count many, const depth_t depth
#ifdef GC_NODES
      , candidate_gc_nodes& gc_nodes
#endif
      )
{
   agg_trie_leaf *leaf(vals.find_configuration(tpl, pred));
   agg_configuration *conf;
   
   if(leaf->get_conf() == NULL) {
      conf = create_configuration();
      leaf->set_conf(conf);
   } else {
      conf = leaf->get_conf();
   }
   
#ifdef DEBUG_AGGS
   cout << "----> Before:" << endl;
   conf->print(cout);
#endif
#ifdef GC_NODES
   conf->add_to_set(tpl, pred, many, depth, gc_nodes);
#else
   conf->add_to_set(tpl, pred, many, depth);
#endif
#ifdef DEBUG_AGGS
   cout << "Add " << *tpl << " " << many << " with depth " << depth << endl;
   conf->print(cout);
#endif
   
   return conf;
}

full_tuple_list
tuple_aggregate::generate(
#ifdef GC_NODES
      candidate_gc_nodes& gc_nodes
#endif
      )
{
   const aggregate_type typ(pred->get_aggregate_type());
   const field_num field(pred->get_aggregate_field());
   full_tuple_list ls;
   
   for(agg_trie::iterator it(vals.begin());
      it != vals.end(); )
   {
      agg_configuration *conf(*it);
      
      assert(conf != NULL);
      
      if(conf->has_changed())
         conf->generate(pred, typ, field, ls
#ifdef GC_NODES
               , gc_nodes
#endif
               );
      
      assert(!conf->has_changed());
      
      if(conf->is_empty())
         it = vals.erase(it, pred
#ifdef GC_NODES
               , gc_nodes
#endif
               );
      else
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
tuple_aggregate::delete_by_index(const match& m
#ifdef GC_NODES
      , candidate_gc_nodes& gc_nodes
#endif
      )
{
   vals.delete_by_index(pred, m
#ifdef GC_NODES
         , gc_nodes
#endif
         );
}

tuple_aggregate::~tuple_aggregate(void)
{
}

void
tuple_aggregate::wipeout(
#ifdef GC_NODES
      candidate_gc_nodes& gc_nodes
#endif
      )
{
   for(agg_trie::const_iterator it(vals.begin());
      it != vals.end();
      it++)
   {
      agg_configuration *conf(*it);
      assert(conf != NULL);
      conf->wipeout(pred
#ifdef GC_NODES
            , gc_nodes
#endif
            );
   }
   vals.wipeout(pred
#ifdef GC_NODES
         , gc_nodes
#endif
         );
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
      assert(conf != NULL);
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
