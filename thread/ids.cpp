
#include <cstdlib>
#include <cstring>

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
ids::commit(void)
{
   if(total_allocated == 0)
      return;
   db::database::map_nodes m;

   node *p(allocated_nodes);
   while(p) {
      if(!p->creator) {
         node *next(p->dyn_next);

         p->deallocate();

         p = next;
      } else {
         m[p->get_id()] = p;
         p = p->dyn_next;
      }
   }
   All->DATABASE->add_node_set(m);
   allocated_nodes = NULL;
   total_allocated = 0;
   deleted_by_others = 0;
}

void
ids::delete_node(node *n)
{
#ifdef GC_NODES
   candidate_gc_nodes gc_nodes;
   n->wipeout(gc_nodes, true);
   assert(gc_nodes.empty());

   if(n->creator == this) {
      node *prev(n->dyn_prev);
      node *next(n->dyn_next);
      if(prev)
         prev->dyn_next = next;
      if(next)
         next->dyn_prev = prev;
      n->deallocate();
      total_allocated--;
   } else {
      ids *creator((ids*)n->creator);
      n->creator = nullptr;
      creator->deleted_by_others++;
   }
#endif
}

void
ids::free_destroyed_nodes()
{
   db::node *p(allocated_nodes);
   size_t by_others{0};

   while(p) {
      if(!p->creator) {
         node *prev(p->dyn_prev);
         node *next(p->dyn_next);
         if(prev)
            prev->dyn_next = next;
         if(next)
            next->dyn_prev = prev;

         p->deallocate();

         if(p == allocated_nodes)
            allocated_nodes = next;
         total_allocated--;
         by_others++;
         p = next;
      } else
         p = p->dyn_next;
   }
   deleted_by_others -= by_others;
}

node*
ids::create_node(void)
{
   if(total_allocated > 0 && deleted_by_others > 0 &&
         deleted_by_others > total_allocated/2)
      free_destroyed_nodes();

   if(next_available_id == end_available_id) {
      next_allocation *= 4;
      allocate_more_ids();
   }

   node *n(node::create(next_available_id, next_translated_id));
   n->remove_temporary_priority();
   n->creator = this;
   if(allocated_nodes)
      allocated_nodes->dyn_prev = n;
   n->dyn_next = allocated_nodes;
   allocated_nodes = n;

   next_available_id++;
   next_translated_id++;
   total_allocated++;

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
