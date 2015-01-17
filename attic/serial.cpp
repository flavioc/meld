
#include "sched/serial.hpp"
#include "process/remote.hpp"
#include "sched/common.hpp"

using namespace db;
using namespace vm;
using namespace process;
using namespace std;

namespace sched
{

void
serial_local::new_agg(work& w)
{
   db::simple_tuple *stpl(w.get_tuple());
   new_work(w.get_node(), w.get_node(), stpl->get_tuple(), stpl->get_predicate(), stpl->get_count(), stpl->get_depth());
   delete stpl;
}

void
serial_local::new_work(node *from, node *target, vm::tuple *tpl, vm::predicate *pred, const ref_count count, const depth_t depth)
{
   (void)from;
   serial_node *to(dynamic_cast<serial_node*>(target));
   
   to->add_work_myself(tpl, pred, count, depth);
   
   if(!to->in_queue()) {
      to->set_in_queue(true);
      queue_nodes.push(to);
   }
}
   
void
serial_local::assert_end(void) const
{
   assert((!has_work() && !stop_flag) || stop_flag);
   assert_static_nodes_end(id);
}

void
serial_local::assert_end_iteration(void) const
{
   assert(!has_work());
   assert_static_nodes_end_iteration(id);
}

node*
serial_local::get_work(void)
{
   if(current_node != NULL) {
      if(!current_node->unprocessed_facts) {
         current_node->set_in_queue(false);
         current_node = NULL;
         if(!has_work())
            return NULL;
			if(!queue_nodes.pop(current_node))
				return NULL;
      }
   } else {
      if(!has_work())
         return NULL;
		if(!queue_nodes.pop(current_node))
			return NULL;
      assert(current_node->unprocessed_facts);
   }
   
   assert(current_node != NULL);
   assert(current_node->unprocessed_facts);
   assert(current_node->in_queue());
   
   return current_node;
}

bool
serial_local::terminate_iteration(void)
{
   generate_aggs();

   return has_work();
}

void
serial_local::generate_aggs(void)
{
   iterate_static_nodes(id);
}

void
serial_local::init(const size_t)
{
   database::map_nodes::iterator it(All->DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(All->DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      serial_node *cur_node(dynamic_cast<serial_node*>(init_node(it)));

      cur_node->set_in_queue(true);
      queue_nodes.push(cur_node);
   }
}

void
serial_local::gather_next_tuples(db::node *node, simple_tuple_list& ls)
{
   (void)node;
   (void)ls;
}

}
