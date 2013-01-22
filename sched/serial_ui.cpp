
#include "process/remote.hpp"
#include "sched/serial_ui.hpp"
#include "sched/common.hpp"
#include "ui/manager.hpp"

#ifdef USE_UI
#include "ui/macros.hpp"
#endif

using namespace db;
using namespace vm;
using namespace process;
using namespace std;

namespace sched
{
	
static serial_ui_local *scheduler(NULL);
	
node*
serial_ui_local::get_work(void)
{
   serial_node* n = (serial_node*)serial_local::get_work();

   while(n != NULL) {
      if(!n->has_work()) {
         n = (serial_node*)serial_local::get_work();
         if(n == NULL)
            continue;
      } else {
         break;
      }
   }

   if(n != NULL) {
      if(first_done) {
         LOG_STEP_DONE(n);
      } else
         first_done = true;
		
      LOG_STEP_START(n);
      if(!WAIT_FOR_NEXT())
         return NULL;
   }
	
	return n;
}

void
serial_ui_local::init(const size_t x)
{
	serial_local::init(x);
	LOG_STEP_DONE(NULL);
}

void
serial_ui_local::end(void)
{
	LOG_PROGRAM_TERMINATION();
	WAIT_FOR_DONE();
}

serial_ui_local::serial_ui_local(vm::all *all):
	serial_local(all),
	first_done(false)
{
	LOG_DATABASE(state.all->DATABASE);
	LOG_PROGRAM(state.all->PROGRAM);
	scheduler = this;
#ifdef USE_UI
   state::UI = true;
#endif
}

#ifdef USE_UI
using namespace json_spirit;

Value
serial_ui_local::dump_this_node(const db::node::node_id id)
{
	node *n(state.all->DATABASE->find_node(id));
	serial_node *snode((serial_node*)n);
	
	Object ret;
	Array q;
	
	// fill queue
	for(serial_node::queue_iterator it(snode->begin()), end(snode->end()); it != end; ++it) {
		node_work w(*it);
		Object tpl;
		simple_tuple *stpl(w.get_tuple());
		UI_ADD_FIELD(tpl, "tuple", stpl->get_tuple()->dump_json());
		UI_ADD_FIELD(tpl, "to_delete", stpl->must_be_deleted());
		UI_ADD_ELEM(q, tpl);
	}
	
	// fill database
	UI_ADD_FIELD(ret, "database", snode->dump_json());

	UI_ADD_FIELD(ret, "queue", q);
	
	return ret;
}

Value
serial_ui_local::dump_node(const db::node::node_id id)
{
	return scheduler->dump_this_node(id);
}

#endif

void
serial_ui_local::change_to_node(serial_node *n)
{
	if(!n->has_work())
		return;
		
	if(current_node) {
		if(current_node->has_work())
			queue_nodes.push(current_node);
		else {
			current_node->set_in_queue(false);
		}
	}
	
	current_node = n;
	assert(current_node->in_queue());

	queue_nodes.remove(current_node);
}

void
serial_ui_local::change_node(const db::node::node_id id)
{
	node *n(scheduler->state.all->DATABASE->find_node(id));
	
	scheduler->change_to_node((serial_node*)n);
}

}
