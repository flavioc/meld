
#include "sched/serial.hpp"
#include "process/remote.hpp"
#include "sched/common.hpp"

using namespace db;
using namespace vm;
using namespace process;

namespace sched
{

void
serial_local::new_agg(work& w)
{
   new_work(w.get_node(), w);
}

void
serial_local::new_work(const node *, work& new_work)
{
   serial_node *to(dynamic_cast<serial_node*>(new_work.get_node()));
   
   node_work n_work(new_work);
   
   to->add_work(n_work);
   
   if(!to->in_queue()) {
      to->set_in_queue(true);
      queue_nodes.push(to);
   }
}
   
void
serial_local::assert_end(void) const
{
   assert(!has_work());
   assert_static_nodes_end(id);
}

void
serial_local::assert_end_iteration(void) const
{
   assert(!has_work());
   assert_static_nodes_end_iteration(id);
}

bool
serial_local::get_work(work& new_work)
{  
   if(current_node != NULL) {
      // holding a node
      if(!current_node->has_work()) {
         current_node->set_in_queue(false);
         current_node = NULL;
         if(!has_work())
            return false;
			if(!queue_nodes.pop(current_node))
				return false;
      }
   } else {
      if(!has_work())
         return false;
		if(!queue_nodes.pop(current_node))
			return false;
      assert(current_node->has_work());
   }
   
   assert(current_node != NULL);
   assert(current_node->has_work());
   assert(current_node->in_queue());
   
   node_work unit(current_node->get_work());
   
   new_work.copy_from_node(current_node, unit);
   
   return true;
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
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      serial_node *cur_node(dynamic_cast<serial_node*>(it->second));
      
      init_node(cur_node);
      
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
}

simple_tuple_list
serial_local::gather_active_tuples(db::node *node, const vm::predicate_id pred)
{
	simple_tuple_list ls;
	serial_node *no((serial_node*)node);
	typedef serial_node::queue_type fact_queue;
	
	for(fact_queue::const_iterator it(no->begin()), end(no->end()); it != end; ++it) {
		process::node_work w(*it);
		simple_tuple *stpl(w.get_tuple());
		
		if(!stpl->must_be_deleted() && stpl->get_predicate_id() == pred) {
			ls.push_back(stpl);
		}
	}
	
	return ls;
}


}
