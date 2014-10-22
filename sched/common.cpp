
#include "sched/common.hpp"
#include "db/database.hpp"
#include "vm/state.hpp"
#include "sched/base.hpp"

using namespace vm;
using namespace db;
using namespace process;

namespace sched
{

#ifndef NDEBUG
void
assert_static_nodes_end_iteration(const process_id id)
{
   if(sched::base::stop_flag)
      return;

   const node::node_id first(All->MACHINE->find_first_node(id));
   const node::node_id final(All->MACHINE->find_last_node(id));
   database::map_nodes::iterator it(All->DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(All->DATABASE->get_node_iterator(final));

   for(; it != end; ++it)
      it->second->assert_end_iteration();
}

void
assert_static_nodes_end(const process_id id)
{
   if(sched::base::stop_flag)
      return;

   const node::node_id first(All->MACHINE->find_first_node(id));
   const node::node_id final(All->MACHINE->find_last_node(id));
   database::map_nodes::iterator it(All->DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(All->DATABASE->get_node_iterator(final));

   for(; it != end; ++it)
      it->second->assert_end();
}
#endif

}
