
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
	
void
serial_ui_local::new_work(const node *from, work& w)
{
	const node *to(w.get_node());
	
	LOG_TUPLE_SEND(from, to, w.get_tuple()->get_tuple());
	
	return serial_local::new_work(from, w);
}

void
serial_ui_local::new_persistent_derivation(node *n, vm::tuple *tpl)
{
	LOG_PERSISTENT_DERIVATION(n, tpl);
}

void
serial_ui_local::new_linear_derivation(node *n, vm::tuple *tpl)
{
	LOG_LINEAR_DERIVATION(n, tpl);
}

void
serial_ui_local::new_linear_consumption(node *n, vm::tuple *tpl)
{
	LOG_LINEAR_CONSUMPTION(n, tpl);
}

bool
serial_ui_local::get_work(work& new_work)
{
	if(first_done) {
		LOG_STEP_DONE(current_node);
		WAIT_FOR_NEXT();
	} else
		first_done = true;
		
	const bool ret(serial_local::get_work(new_work));
	
	if(ret) {
		LOG_STEP_START(new_work.get_node(), new_work.get_tuple()->get_tuple());
	}
	
	return ret;
}

void
serial_ui_local::init(const size_t x)
{
	serial_local::init(x);
	LOG_STEP_DONE(NULL);
	WAIT_FOR_NEXT();
}

void
serial_ui_local::end(void)
{
	LOG_PROGRAM_TERMINATION();
	WAIT_FOR_DONE();
}

void
serial_ui_local::set_node_color(node *n, const int r, const int g, const int b)
{
	LOG_SET_COLOR(n, r, g, b);
}

void
serial_ui_local::set_edge_label(node *from, const node::node_id to, const runtime::rstring::ptr str)
{
	LOG_SET_EDGE_LABEL(from->get_id(), to, str->get_content());
}

serial_ui_local::serial_ui_local(void):
	serial_local(),
	first_done(false)
{
	LOG_DATABASE(state::DATABASE);
	LOG_PROGRAM(state::PROGRAM);
	scheduler = this;
}

#ifdef USE_UI
using namespace json_spirit;

Value
serial_ui_local::dump_this_node(const db::node::node_id id)
{
	node *n(state::DATABASE->find_node(id));
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
	node *n(state::DATABASE->find_node(id));
	
	scheduler->change_to_node((serial_node*)n);
}

}
