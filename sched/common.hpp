
#ifndef SCHED_COMMON_HPP
#define SCHED_COMMON_HPP

#include "vm/defs.hpp"

namespace sched
{

void assert_static_nodes(const vm::process_id);

#define iterate_static_nodes(ID)                                                 \
   const node::node_id first(remote::self->find_first_node(ID));                 \
   const node::node_id final(remote::self->find_last_node(ID));                  \
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));  \
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final)); \
   for(; it != end; ++it)                                                        \
      node_iteration(it->second)
}

#endif
