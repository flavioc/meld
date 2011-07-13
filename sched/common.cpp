
#include "sched/common.hpp"
#include "db/database.hpp"
#include "process/remote.hpp"

using namespace vm;
using namespace db;
using namespace process;

namespace sched
{
   
void
assert_static_nodes(const process_id id)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));

   for(; it != end; ++it)
      it->second->assert_end();
}

}
