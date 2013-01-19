
#ifndef SCHED_COMMON_HPP
#define SCHED_COMMON_HPP

#include "vm/defs.hpp"
#include "vm/all.hpp"

namespace sched
{

#ifdef NDEBUG
inline void assert_static_nodes_end_iteration(const vm::process_id, vm::all *) {}
inline void assert_static_nodes_end(const vm::process_id, vm::all *) {}
#else
void assert_static_nodes_end_iteration(const vm::process_id, vm::all *);
void assert_static_nodes_end(const vm::process_id, vm::all *);
#endif

#define iterate_static_nodes(ID)                                                       \
   const node::node_id first(remote::self->find_first_node(ID));                       \
   const node::node_id final(remote::self->find_last_node(ID));                        \
   database::map_nodes::iterator it(state.all->DATABASE->get_node_iterator(first));    \
   database::map_nodes::iterator end(state.all->DATABASE->get_node_iterator(final));   \
   for(; it != end; ++it)                                                              \
      node_iteration(it->second)
}

#endif
