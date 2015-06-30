
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

db::node*
ids::remove_node_from_allocated(db::node *n)
{
   node *prev(n->dyn_prev);
   node *next(n->dyn_next);
   if(prev)
      prev->dyn_next = next;
   if(next)
      next->dyn_prev = prev;
   if(n == allocated_nodes)
      allocated_nodes = next;
   if(total_freed == 32)
      n->deallocate();
   else {
      total_freed++;
      n->dyn_prev = NULL;
      n->dyn_next = freed_nodes;
      if(freed_nodes)
         freed_nodes->dyn_prev = n;
      freed_nodes = n;
   }
   total_allocated--;
   return next;
}

void
ids::delete_node(node *n)
{
   (void)n;
#ifdef GC_NODES
   candidate_gc_nodes gc_nodes;
   n->wipeout(gc_nodes, true);
   assert(gc_nodes.empty());

   if(n->creator == this) {
      remove_node_from_allocated(n);
   } else {
      ids *creator((ids*)n->creator.load());
      n->creator = nullptr;
      creator->deleted_by_others++;
      // will be deleted later...
   }
#endif
}

void
ids::free_destroyed_nodes()
{
   db::node *p(allocated_nodes);
   size_t by_others{0};

   while(p) {
      if(!p->creator)
         p = remove_node_from_allocated(p);
      else
         p = p->dyn_next;
   }
   deleted_by_others -= by_others;
}

void
ids::add_allocated_node(db::node *n)
{
   n->remove_temporary_priority();
   n->creator = this;
   if(allocated_nodes)
      allocated_nodes->dyn_prev = n;
   n->dyn_next = allocated_nodes;
   allocated_nodes = n;

   total_allocated++;
}

node*
ids::create_node(void)
{
   if(total_allocated > 0 && deleted_by_others > 0 &&
         deleted_by_others > total_allocated/2)
      free_destroyed_nodes();

   if(freed_nodes) {
      total_freed--;
      db::node *n(freed_nodes);
      freed_nodes = n->dyn_next;
      if(freed_nodes)
         n->dyn_prev = nullptr;
      n->dyn_next = nullptr;
      n->dyn_prev = nullptr;
      auto id(n->get_id());
      auto translate(n->get_translated_id());
      mem::allocator<node>().construct(n, id, translate);
      add_allocated_node(n);

      return n;
   }

   if(next_available_id == end_available_id) {
      next_allocation *= 4;
      allocate_more_ids();
   }

   node *n(node::create(next_available_id, next_translated_id));
   add_allocated_node(n);

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
