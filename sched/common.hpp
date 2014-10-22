
#ifndef SCHED_COMMON_HPP
#define SCHED_COMMON_HPP

#include "vm/defs.hpp"
#include "vm/all.hpp"

namespace sched
{

#ifdef NDEBUG
inline void assert_static_nodes_end_iteration(const vm::process_id) {}
inline void assert_static_nodes_end(const vm::process_id) {}
#else
void assert_static_nodes_end_iteration(const vm::process_id);
void assert_static_nodes_end(const vm::process_id);
#endif

#define iterate_static_nodes(ID)                                                       \
   const node::node_id first(All->MACHINE->find_first_node(ID));                       \
   const node::node_id final(All->MACHINE->find_last_node(ID));                        \
   database::map_nodes::iterator it(vm::All->DATABASE->get_node_iterator(first));    \
   database::map_nodes::iterator end(vm::All->DATABASE->get_node_iterator(final));   \
   for(; it != end; ++it)                                                              \
      node_iteration(it->second)
}

#endif
