
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
vm::predicate* sim_sched::neighbor_pred(NULL);
vm::predicate* sim_sched::tap_pred(NULL);
bool sim_sched::thread_mode(false);
bool sim_sched::stop_all(false);
queue::push_safe_linear_queue<sim_sched::message_type*> sim_sched::socket_messages;
	
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
	if(slave)
		return;
		
	assert(num_threads == 1);
	
   database::map_nodes::iterator it(state.all->DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state.all->DATABASE->get_node_iterator(remote::self->find_last_node(id)));

	// no nodes
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
	
	if(thread_mode) {
		to->pending.push(stpl);
	} else {
		heap_priority pr;
		if(current_node == NULL) {
			pr.int_priority = 0;
			to->tuple_pqueue.insert(stpl, pr);
		} else {
			tmp_work.push_back(new_work);
		}
	}
}

node*
sim_sched::get_work(void)
{
	if(slave) {
		while(current_node->pending.empty()) {
			usleep(100);
			if(stop_all)
				return NULL;
		}
		assert(!current_node->pending.empty());
		return current_node;
	}
	
	assert(!thread_mode && !slave);
	
#define CREATE_N_NODES 1
#define RUN_NODE 2
#define NODE_RUN 3
#define STOP 4
#define ADD_NEIGHBOR 5
#define REMOVE_NEIGHBOR 6
#define TAP 7
#define SET_COLOR 8
#define USE_THREADS 9
	message_type reply[MAXLENGTH];
	
	if(current_node) {
		// we just did a round of computation
		assert(!thread_mode);
		
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
		reply[i++] = (4 + nodes.size()) * sizeof(message_type);
		reply[i++] = NODE_RUN;
		reply[i++] = (message_type)current_node->get_id();
		reply[i++] = (message_type)pr.int_priority;
		reply[i++] = (message_type)nodes.size();
		
		for(set<sim_node*>::iterator it(nodes.begin()), end(nodes.end());
			it != end;
			++it)
		{
			reply[i++] = (message_type)(*it)->get_id();
		}
	
		boost::asio::write(*socket, boost::asio::buffer(reply, reply[0] + sizeof(message_type)));
		current_node = NULL;
	}
	
	while(true) {
		
		if(!socket->available()) {
			while(!socket_messages.empty()) {
				message_type *data(socket_messages.pop());
				boost::asio::write(*socket, boost::asio::buffer(data, data[0] + sizeof(message_type)));
				delete []data;
			}
			usleep(100);
			continue;
		}
		
		size_t length =
			socket->read_some(boost::asio::buffer(reply, sizeof(message_type)));
		assert(length == sizeof(message_type));
		length = socket->read_some(boost::asio::buffer(reply+1,  reply[0]));

		assert(length == (size_t)reply[0]);
		
		switch(reply[1]) {
			case USE_THREADS: {
				thread_mode = true;
			}
			break;
			case CREATE_N_NODES:
				{
					message_type n(reply[2]);
					message_type ts(reply[3]);
					
					for(message_type i(0); i != n; ++i) {
						db::node *no(state.all->DATABASE->create_node());
						init_node(no);
						if(thread_mode) {
							sim_sched *th(new sim_sched(state.all, state.all->NUM_THREADS, (sim_node*)no));
							no->set_owner(th);
							state.all->MACHINE->init_thread(th);
						}
					}
					
					cout << "Create " << n << " nodes" << endl;
				}
				break;
			case RUN_NODE: {
				message_type node(reply[2]);
				message_type until(reply[3]);
				
				assert(!thread_mode);
				
				//cout << "Run node " << node << " until " << until << endl;
				db::node *no(state.all->DATABASE->find_node((db::node::node_id)node));
				
				current_node = (sim_node*)no;
				current_node->timestamp = (size_t)until;
				state.sim_instr_counter = 0;
				
				return no;
			}
			break;
			case ADD_NEIGHBOR: {
				message_type in(reply[2]);
				message_type out(reply[3]);
				message_type side(reply[4]);
				message_type ts(reply[5]);
				
				sim_node *no_in(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(in)));
				
				vm::tuple *tpl(new vm::tuple(neighbor_pred));
				tpl->set_node(0, out);
				tpl->set_int(1, side);
				
				db::simple_tuple *stpl(new db::simple_tuple(tpl, 1));
				
				if(thread_mode) {
					no_in->pending.push(stpl);
				} else {
					heap_priority pr;
					pr.int_priority = ts;
					no_in->tuple_pqueue.insert(stpl, pr);
				}
			}
			break;
			case REMOVE_NEIGHBOR: {
				message_type in(reply[2]);
				message_type out(reply[3]);
				message_type side(reply[4]);
				message_type ts(reply[5]);
				
				sim_node *no_in(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(in)));
				
				vm::tuple *tpl(new vm::tuple(neighbor_pred));
				tpl->set_node(0, out);
				tpl->set_int(1, side);
				
				db::simple_tuple *stpl(new db::simple_tuple(tpl, -1));
				
				if(thread_mode) {
					no_in->pending.push(stpl);
				} else {
					heap_priority pr;
					pr.int_priority = ts;
					no_in->tuple_pqueue.insert(stpl, pr);
				}
			}
			break;
			case TAP: {
				message_type node(reply[2]);
				message_type ts(reply[3]);
				
				sim_node *no(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(node)));
				
				vm::tuple *tpl(new vm::tuple(tap_pred));
				db::simple_tuple *stpl(new db::simple_tuple(tpl, 1));
				
				if(thread_mode) {
					no->pending.push(stpl);
				} else {
					heap_priority pr;
					pr.int_priority = ts;
					no->tuple_pqueue.insert(stpl, pr);
				}
			}
			break;
			case STOP: {
				//cout << "STOP" << endl;
				stop_all = true;
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
	message_type *data = new message_type[6];
	
	data[0] = 5 * sizeof(message_type);
	data[1] = SET_COLOR;
	data[2] = (message_type)n->get_id();
	data[3] = (message_type)r;
	data[4] = (message_type)g;
	data[5] = (message_type)b;
	
	if(thread_mode) {
		socket_messages.push(data);
	} else {
		boost::asio::write(*socket, boost::asio::buffer(data, data[0] + sizeof(message_type)));
		delete []data;
	}
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
	
	no->pending.pop_list(ls);
}

}