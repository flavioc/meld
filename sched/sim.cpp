
#include <boost/asio.hpp>
#include <string>

#include "sched/sim.hpp"
#include "db/database.hpp"
#include "process/remote.hpp"
#include "sched/common.hpp"
#include "process/machine.hpp"
#include "utils/utils.hpp"

using namespace db;
using namespace vm;
using namespace process;
using boost::asio::ip::tcp;
using namespace std;

namespace sched
{
	
int sim_sched::PORT(0);
	
sim_sched::~sim_sched(void)
{
	if(socket != NULL) {
		//socket->close();
		//delete socket;
	}
}
	
void
sim_sched::init(const size_t num_threads)
{
	assert(num_threads == 1);
	
   database::map_nodes::iterator it(state.all->DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state.all->DATABASE->get_node_iterator(remote::self->find_last_node(id)));

	assert(it == end);
	
	state::SIM = true;
	
	try {
   	// add socket
		boost::asio::io_service io_service;
		
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(tcp::v4(), "127.0.0.1", utils::to_string(PORT));
		tcp::resolver::iterator iterator = resolver.resolve(query);
	
		socket = new tcp::socket(io_service);
		socket->connect(*iterator);
	} catch(std::exception &e) {
		throw machine_error("can't connect to simulator");
	}
	
	// find neighbor predicate
	neighbor_pred = state.all->PROGRAM->get_predicate_by_name("neighbor");
	assert(neighbor_pred != NULL);
	assert(neighbor_pred->num_fields() == 2);
	
	tap_pred = state.all->PROGRAM->get_predicate_by_name("tap");
	assert(tap_pred != NULL);
	assert(tap_pred->num_fields() == 0);
}

void
sim_sched::new_work(const node *, work& new_work)
{
   sim_node *to(dynamic_cast<sim_node*>(new_work.get_node()));
   
	db::simple_tuple *stpl(new_work.get_tuple());
	
	heap_priority pr;
	if(current_node == NULL) {
		pr.int_priority = 0;
		to->tuple_pqueue.insert(stpl, pr);
	} else {
		tmp_work.push_back(new_work);
	}
}

static const size_t MAXLENGTH = 128;

node*
sim_sched::get_work(void)
{
#define CREATE_N_NODES 1
#define RUN_NODE 2
#define NODE_RUN 3
#define STOP 4
#define ADD_NEIGHBOR 5
#define REMOVE_NEIGHBOR 6
#define TAP 7
#define SET_COLOR 8
	int reply[MAXLENGTH];
	
	if(current_node) {
		// we just did a round of computation
		size_t time_spent(state.sim_instr_counter);
		std::set<sim_node*> nodes;
		heap_priority pr;
		
		//cout << "Current node ts " << current_node->timestamp << " spent " << time_spent << endl;
		
		pr.int_priority = current_node->timestamp + time_spent;
		
		for(list<process::work>::iterator it(tmp_work.begin()), end(tmp_work.end());
			it != end;
			++it)
		{
			sim_node *no(dynamic_cast<sim_node*>(it->get_node()));
			nodes.insert(no);
			no->tuple_pqueue.insert(it->get_tuple(), pr);
		}
		tmp_work.clear();
		
		size_t i(0);
		reply[i++] = (4 + nodes.size()) * sizeof(int);
		reply[i++] = NODE_RUN;
		reply[i++] = (int)current_node->get_id();
		reply[i++] = pr.int_priority;
		reply[i++] = (int)nodes.size();
		
		for(set<sim_node*>::iterator it(nodes.begin()), end(nodes.end());
			it != end;
			++it)
		{
			reply[i++] = (int)(*it)->get_id();
		}
	
		boost::asio::write(*socket, boost::asio::buffer(reply, reply[0] + sizeof(int)));
		current_node = NULL;
	}
	
	while(true) {
		size_t length =
			socket->read_some(boost::asio::buffer(reply, sizeof(int)));
		assert(length == sizeof(int));
		length = socket->read_some(boost::asio::buffer(reply+1,  reply[0]));

		assert(length == reply[0]);
		
		switch(reply[1]) {
			case CREATE_N_NODES:
				{
					int n(reply[2]);
					int ts(reply[3]);
					
					for(size_t i(0); i != n; ++i) {
						db::node *no(state.all->DATABASE->create_node());
						init_node(no);
					}
					
					cout << "Create " << n << " nodes" << endl;
				}
				break;
			case RUN_NODE: {
				int node(reply[2]);
				int until(reply[3]);
				
				//cout << "Run node " << node << " until " << until << endl;
				db::node *no(state.all->DATABASE->find_node((db::node::node_id)node));
				
				current_node = (sim_node*)no;
				current_node->timestamp = (size_t)until;
				state.sim_instr_counter = 0;
				
				return no;
			}
			break;
			case ADD_NEIGHBOR: {
				int in(reply[2]);
				int out(reply[3]);
				int side(reply[4]);
				int ts(reply[5]);
				
				sim_node *no_in(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(in)));
				
				heap_priority pr;
				pr.int_priority = ts;
				vm::tuple *tpl(new vm::tuple(neighbor_pred));
				tpl->set_node(0, out);
				tpl->set_int(1, side);
				
				db::simple_tuple *stpl(new db::simple_tuple(tpl, 1));
				
				no_in->tuple_pqueue.insert(stpl, pr);
			}
			break;
			case REMOVE_NEIGHBOR: {
				int in(reply[2]);
				int out(reply[3]);
				int side(reply[4]);
				int ts(reply[5]);
				
				sim_node *no_in(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(in)));
				
				heap_priority pr;
				pr.int_priority = ts;
				vm::tuple *tpl(new vm::tuple(neighbor_pred));
				tpl->set_node(0, out);
				tpl->set_int(1, side);
				
				db::simple_tuple *stpl(new db::simple_tuple(tpl, -1));
				
				no_in->tuple_pqueue.insert(stpl, pr);
			}
			break;
			case TAP: {
				int node(reply[2]);
				int ts(reply[3]);
				
				sim_node *no(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(node)));
				
				heap_priority pr;
				pr.int_priority = ts;
				vm::tuple *tpl(new vm::tuple(tap_pred));
				db::simple_tuple *stpl(new db::simple_tuple(tpl, 1));
				
				no->tuple_pqueue.insert(stpl, pr);
			}
			break;
			case STOP: {
				//cout << "STOP" << endl;
				return NULL;
			}
		}
	}
	assert(false);
	return NULL;
}

void
sim_sched::set_color(db::node *n, const int r, const int g, const int b)
{
	int data[MAXLENGTH];
	
	data[0] = 5 * sizeof(int);
	data[1] = SET_COLOR;
	data[2] = n->get_id();
	data[3] = r;
	data[4] = g;
	data[5] = b;
	
	boost::asio::write(*socket, boost::asio::buffer(data, data[0] + sizeof(int)));
}

void
sim_sched::new_agg(work& w)
{
   new_work(w.get_node(), w);
}

void
sim_sched::generate_aggs(void)
{
	iterate_static_nodes(id);
}

bool
sim_sched::terminate_iteration(void)
{
   generate_aggs();

	return false;
}

void
sim_sched::assert_end(void) const
{
   assert_static_nodes_end(id, state.all);
}

void
sim_sched::assert_end_iteration(void) const
{
	assert_static_nodes_end_iteration(id, state.all);
}

simple_tuple_vector
sim_sched::gather_active_tuples(db::node *node, const vm::predicate_id pred)
{
	return simple_tuple_vector();
}

void
sim_sched::gather_next_tuples(db::node *node, simple_tuple_list& ls)
{
	sim_node *no((sim_node*)node);
	
	heap_priority pr;
	
	// we want to get all the tuples until no->timestamp
	
	while(!no->tuple_pqueue.empty()) {
		pr = no->tuple_pqueue.min_value();
		
		if(pr.int_priority > no->timestamp)
			break;
		
		db::simple_tuple *stpl(no->tuple_pqueue.pop());
		ls.push_back(stpl);
	}
}

}