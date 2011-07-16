
#include "sched/global/serial.hpp"
#include "process/remote.hpp"
#include "vm/state.hpp"
#include "sched/common.hpp"

using namespace db;
using namespace vm;
using namespace process;

namespace sched
{

void
serial_global::new_work(const node *, work& new_work)
{
   queue_work.push(new_work, new_work.get_strat_level());
}

void
serial_global::assert_end_iteration(void) const
{
   assert(!has_work());
   assert_static_nodes_end_iteration(id);
}

void
serial_global::assert_end(void) const
{
   assert(!has_work());
   assert_static_nodes_end(id);
}

void
serial_global::end(void)
{
   assert(!has_work());
}

bool
serial_global::get_work(work& new_work)
{
   if(!has_work())
      return false;
   
   new_work = queue_work.pop();
   
   return true;
}

void
serial_global::generate_aggs(void)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));

   for(; it != end; ++it)
      node_iteration(it->second);
}

bool
serial_global::terminate_iteration(void)
{
   generate_aggs();
   
   return has_work();
}

void
serial_global::init(const size_t)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));
   
   for(; it != end; ++it)
      init_node(it->second);
}

}