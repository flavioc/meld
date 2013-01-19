
#include "sched/common.hpp"
#include "db/database.hpp"
#include "process/remote.hpp"
#include "vm/state.hpp"

using namespace vm;
using namespace db;
using namespace process;

namespace sched
{

#ifndef NDEBUG
void
assert_static_nodes_end_iteration(const process_id id, vm::all *all)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(all->DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(all->DATABASE->get_node_iterator(final));

   for(; it != end; ++it)
      it->second->assert_end_iteration();
}

void
assert_static_nodes_end(const process_id id, vm::all *all)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(all->DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(all->DATABASE->get_node_iterator(final));

   for(; it != end; ++it)
      it->second->assert_end();
}
#endif

}
