
#include "vm/all.hpp"
#include "db/node.hpp"
#include "db/database.hpp"
#include "thread/ids.hpp"

using namespace std;
using namespace db;
using namespace vm;

namespace sched
{

void
ids::merge(ids& other)
{
   for(auto & elem : other.added_nodes) {
      auto it2(removed_nodes.find(elem.first));
      if(it2 == removed_nodes.end())
         added_nodes.insert(elem);
      else
         removed_nodes.erase(it2);
   }
   other.added_nodes.clear();

   for(const auto & elem : other.removed_nodes) {
      auto it2(added_nodes.find(elem));

      if(it2 == added_nodes.end())
         removed_nodes.insert(elem);
      else
         added_nodes.erase(it2);
   }
   other.removed_nodes.clear();
}

void
ids::commit(void)
{
   All->DATABASE->add_node_set(added_nodes);
   assert(removed_nodes.empty());
   added_nodes.clear();
}

void
ids::delete_node(node *n)
{
#ifdef GC_NODES
   auto it(added_nodes.find(n->get_id()));

   if(it != added_nodes.end())
      added_nodes.erase(it);
   else
      removed_nodes.insert(n->get_id());

   candidate_gc_nodes gc_nodes;
   n->wipeout(gc_nodes);
   assert(gc_nodes.empty());
#endif
}

node*
ids::create_node(void)
{
   if(next_available_id == end_available_id) {
      next_allocation *= 2;
      allocate_more_ids();
   }

   node *n(node::create(next_available_id, next_translated_id));
   n->remove_temporary_priority();

   added_nodes[next_available_id] = n;
   next_available_id++;
   next_translated_id++;

   return n;
}

void
ids::allocate_more_ids(void)
{
   pair<node::node_id, node::node_id> p(All->DATABASE->allocate_ids(next_allocation));
   next_available_id = p.first;
   next_translated_id = p.second;
   end_available_id = next_available_id + next_allocation;
}

ids::ids(void):
   next_allocation(ALLOCATED_IDS)
{
   allocate_more_ids();
}

}
