
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

#define CREATE_N_NODES 1
#define RUN_NODE 2
#define NODE_RUN 3
#define STOP 4
#define ADD_NEIGHBOR 5
#define REMOVE_NEIGHBOR 6
#define TAP 7
#define SET_COLOR 8
#define USE_THREADS 9
#define SEND_MESSAGE 12
#define RECEIVE_MESSAGE 13
#define ACCEL 14
#define SHAKE 15

// debug messages for simulation
#define DEBUG

namespace sched
{
	
int sim_sched::PORT(0);
vm::predicate* sim_sched::neighbor_pred(NULL);
vm::predicate* sim_sched::tap_pred(NULL);
vm::predicate* sim_sched::neighbor_count_pred(NULL);
vm::predicate* sim_sched::accel_pred(NULL);
vm::predicate* sim_sched::shake_pred(NULL);
vm::predicate* sim_sched::vacant_pred(NULL);
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
   if(neighbor_pred) {
      assert(neighbor_pred->num_fields() == 2);
   } else {
      cerr << "No neighbor predicate found" << endl;
   }
	
	tap_pred = state.all->PROGRAM->get_predicate_by_name("tap");
   if(tap_pred) {
      assert(tap_pred->num_fields() == 0);
   } else {
      cerr << "No tap predicate found" << endl;
   }

   neighbor_count_pred = state.all->PROGRAM->get_predicate_by_name("neighborCount");
   if(neighbor_count_pred) {
      assert(neighbor_count_pred->num_fields() == 1);
   } else {
      cerr << "No neighbor_count predicate found" << endl;
   }

   accel_pred = state.all->PROGRAM->get_predicate_by_name("accel");
   if(accel_pred) {
      assert(accel_pred->num_fields() == 1);
   } else {
      cerr << "No accel predicate found" << endl;
   }

   shake_pred = state.all->PROGRAM->get_predicate_by_name("shake");
   if(shake_pred) {
      assert(shake_pred->num_fields() == 3);
   } else {
      cerr << "No shake predicate found" << endl;
   }

   vacant_pred = state.all->PROGRAM->get_predicate_by_name("vacant");
   if(vacant_pred) {
      assert(vacant_pred->num_fields() == 1);
   } else {
      cerr << "No vacant predicate found" << endl;
   }
}

void
sim_sched::new_work(const node *_src, work& new_work)
{
   sim_node *to(dynamic_cast<sim_node*>(new_work.get_node()));
   
	db::simple_tuple *stpl(new_work.get_tuple());
	
	if(thread_mode) {
		to->pending.push(stpl);
	} else {
		if(current_node == NULL) {
         heap_priority pr;
			pr.int_priority = 0; // this is the init tuple...
         if(stpl->get_count() > 0)
            to->tuple_pqueue.insert(stpl, pr);
         else
            to->rtuple_pqueue.insert(stpl, pr);
		} else {
         sim_node *src(dynamic_cast<sim_node*>((node*)_src));
         const size_t timestamp(src->timestamp + state.sim_instr_counter);
         work_info info;
         info.work = new_work;
         info.timestamp = timestamp;
         info.src = src;

			tmp_work.push_back(info);
		}
	}
}

void
sim_sched::send_pending_messages(void)
{
   while(!socket_messages.empty()) {
      message_type *data(socket_messages.pop());
      boost::asio::write(*socket, boost::asio::buffer(data, data[0] + sizeof(message_type)));
      delete []data;
   }
}

void
sim_sched::add_received_tuple(sim_node *no, size_t ts, db::simple_tuple *stpl)
{
   if(thread_mode) {
      no->pending.push(stpl);
   } else {
      heap_priority pr;
      pr.int_priority = ts;
      if(stpl->get_count() > 0)
         no->tuple_pqueue.insert(stpl, pr);
      else
         no->rtuple_pqueue.insert(stpl, pr);
   }
}

void
sim_sched::add_neighbor(const size_t ts, sim_node *no, const node_val out, const int side, const int count)
{
   if(!neighbor_pred)
      return;

   vm::tuple *tpl(new vm::tuple(neighbor_pred));
   tpl->set_node(0, out);
   tpl->set_int(1, side);
				
   db::simple_tuple *stpl(new db::simple_tuple(tpl, count));
				
   add_received_tuple(no, ts, stpl);
}

void
sim_sched::add_neighbor_count(const size_t ts, sim_node *no, const size_t total, const int count)
{
   if(!neighbor_count_pred)
      return;

   vm::tuple *tpl(new vm::tuple(neighbor_count_pred));
   tpl->set_int(0, (int_val)total);

   db::simple_tuple *stpl(new db::simple_tuple(tpl, count));

   add_received_tuple(no, ts, stpl);
}

void
sim_sched::add_vacant(const size_t ts, sim_node *no, const int side, const int count)
{
   if(!vacant_pred)
      return;

   vm::tuple *tpl(new vm::tuple(vacant_pred));
   tpl->set_int(0, side);

   db::simple_tuple *stpl(new db::simple_tuple(tpl, count));

   add_received_tuple(no, ts, stpl);
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
	
	message_type reply[MAXLENGTH];
	
	if(current_node) {
		// we just did a round of computation
		assert(!thread_mode);
		
		std::set<sim_node*> nodes; // all nodes touched

      // go through all new work and pack several SEND_MESSAGE messages
      // so we can send it all at once

		for(list<work_info>::iterator it(tmp_work.begin()), end(tmp_work.end());
			it != end;
			++it)
		{
         db::simple_tuple *stpl(it->work.get_tuple());
         const size_t stpl_size(stpl->storage_size());
         const size_t msg_size = 4 * sizeof(message_type) + stpl_size;
			sim_node *no(dynamic_cast<sim_node*>(it->work.get_node()));
         size_t i = 0;
         reply[i++] = (message_type)msg_size;
         reply[i++] = SEND_MESSAGE;
         reply[i++] = (message_type)it->timestamp;
         reply[i++] = (message_type)it->src->get_id();
         reply[i++] = (message_type)it->src->get_face(no->get_id());

         int pos = i * sizeof(message_type);
         stpl->pack((utils::byte*)reply, msg_size + sizeof(message_type), &pos);

         assert((size_t)pos == msg_size + sizeof(message_type));
         cout << "Sent message " << *stpl << endl;

         simple_tuple::wipeout(stpl);

         boost::asio::write(*socket, boost::asio::buffer(reply, reply[0] + sizeof(message_type)));

         nodes.insert(no);
		}
		tmp_work.clear();
		
		size_t time_spent(state.sim_instr_counter);
      size_t i(0);
		reply[i++] = (4 + nodes.size()) * sizeof(message_type);
		reply[i++] = NODE_RUN;
		reply[i++] = (message_type)(current_node->timestamp + time_spent); // timestamp
		reply[i++] = (message_type)current_node->get_id();
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
         send_pending_messages();
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
			  cout << "Run in threads mode" << endl;
				thread_mode = true;
			}
			break;
			case CREATE_N_NODES:
				{
					message_type ts(reply[2]);
               message_type node(reply[3]);
					message_type n(reply[4]);
               message_type start_id(reply[5]);
               (void)node; // we dont care about it
#ifdef DEBUG
               cout << "Create " << n << " nodes from " << start_id << endl;
#endif
					for(message_type i(0); i != n; ++i) {
						db::node *no(state.all->DATABASE->create_node_id(start_id + i));
						init_node(no);
						if(thread_mode) {
							sim_sched *th(new sim_sched(state.all, state.all->NUM_THREADS, (sim_node*)no));
							no->set_owner(th);
							state.all->MACHINE->init_thread(th);
						}
                  for(int side = 0; side <= 5; ++side) {
                     add_vacant(ts, (sim_node *)no, side, 1);
                  }
                  add_neighbor_count(ts, (sim_node *)no, 0, 1);
					}
				}
				break;
			case RUN_NODE: {
				message_type until(reply[2]);
				message_type node(reply[3]);

#ifdef DEBUG
            cout << "Run node " << node << " until " << until << endl;
#endif
				
				assert(!thread_mode);
				
				//cout << "Run node " << node << " until " << until << endl;
				db::node *no(state.all->DATABASE->find_node((db::node::node_id)node));
				
				current_node = (sim_node*)no;
				current_node->timestamp = (size_t)until;
				state.sim_instr_counter = 0;
				
				return no;
			}
			break;
         case RECEIVE_MESSAGE: {
            assert(!thread_mode);
            message_type ts(reply[2]);
            message_type node(reply[3]);
            message_type face(reply[4]);
            sim_node *origin(dynamic_cast<sim_node*>(state.all->DATABASE->find_node((db::node::node_id)node)));
            sim_node *target(NULL);

            if(face == (message_type)-1) {
               target = origin;
            } else {
               target = dynamic_cast<sim_node*>(state.all->DATABASE->find_node((db::node::node_id)*(origin->get_node_at_face(face))));
            }

            int pos(5 * sizeof(message_type));
            simple_tuple *stpl(simple_tuple::unpack((utils::byte*)reply, reply[0] + sizeof(message_type), &pos, state.all->PROGRAM));

#ifdef DEBUG
            cout << "Receive message " << origin->get_id() << " to " << target->get_id() << " " << *stpl << endl;
#endif

            heap_priority pr;
            pr.int_priority = ts;
            target->tuple_pqueue.insert(stpl, pr);
         }
         break;
			case ADD_NEIGHBOR: {
				message_type ts(reply[2]);
				message_type in(reply[3]);
				node_val out((node_val)reply[4]);
				int side((int)reply[5]);

#ifdef DEBUG
            cout << ts << " neighbor(" << in << ", " << out << ", " << side << ")" << endl;
#endif
				
				sim_node *no_in(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(in)));
            node_val *face(no_in->get_node_at_face(side));

            if(*face == -1) {
               // remove vacant first, add 1 to neighbor count
               add_vacant(ts, no_in, side, -1);
               add_neighbor_count(ts, no_in, no_in->neighbor_count, -1);
               no_in->neighbor_count++;
#ifdef DEBUG
               cout << in << " neighbor count went up to " << no_in->neighbor_count << endl;
#endif
               add_neighbor_count(ts, no_in, no_in->neighbor_count, 1);
               *face = out;
               add_neighbor(ts, no_in, out, side, 1);
            } else {
               if(*face != out) {
                  // remove old node
                  add_neighbor(ts, no_in, *face, side, -1);
                  *face = out;
                  add_neighbor(ts, no_in, out, side, 1);
               }
            }
			}
			break;
         case REMOVE_NEIGHBOR: {
				message_type ts(reply[2]);
				message_type in(reply[3]);
				int side((int)reply[4]);

#ifdef DEBUG
            cout << ts << " remove neighbor(" << in << ", " << side << ")" << endl;
#endif
				
				sim_node *no_in(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(in)));
            node_val *face(no_in->get_node_at_face(side));

            if(*face == -1) {
               // remove vacant first, add 1 to neighbor count
               cerr << "Current face is vacant, cannot remove node!" << endl;
               assert(false);
            } else {
               // remove old node
               add_neighbor_count(ts, no_in, no_in->neighbor_count, -1);
               no_in->neighbor_count--;
               add_neighbor_count(ts, no_in, no_in->neighbor_count, 1);
            }

            add_neighbor(ts, no_in, *face, side, -1);

            *face = -1; // now vacant
          }
          break;
			case TAP: {
				message_type ts(reply[2]);
				message_type node(reply[3]);

            cout << ts << " tap(" << node << ")" << endl;
				
				sim_node *no(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(node)));

            if(tap_pred) {
               vm::tuple *tpl(new vm::tuple(tap_pred));
               db::simple_tuple *stpl(new db::simple_tuple(tpl, 1));
               
               add_received_tuple(no, ts, stpl);
            }
			}
			break;
         case ACCEL: {
            message_type ts(reply[2]);
            message_type node(reply[3]);
            message_type f(reply[4]);

            cout << ts << " accel(" << node << ", " << f << ")" << endl;

            sim_node *no(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(node)));

            if(accel_pred) {
               vm::tuple *tpl(new vm::tuple(accel_pred));
               tpl->set_int(0, (int_val)f);

               db::simple_tuple *stpl(new db::simple_tuple(tpl, 1));

               add_received_tuple(no, ts, stpl);
            }
         }
         break;
         case SHAKE: {
            message_type ts(reply[2]);
            message_type node(reply[3]);
            message_type x(reply[4]);
            message_type y(reply[5]);
            message_type z(reply[6]);

            cout << ts << " shake(" << node << ", " << x << ", " << y << ", " << z << ")" << endl;

            sim_node *no(dynamic_cast<sim_node*>(state.all->DATABASE->find_node(node)));

            if(shake_pred) {
               vm::tuple *tpl(new vm::tuple(shake_pred));
               tpl->set_int(0, (int_val)x);
               tpl->set_int(1, (int_val)y);
               tpl->set_int(2, (int_val)z);

               db::simple_tuple *stpl(new db::simple_tuple(tpl, 1));

               add_received_tuple(no, ts, stpl);
            }
         }
         break;
			case STOP: {
				//cout << "STOP" << endl;
				stop_all = true;
            sleep(1);
            send_pending_messages();
            usleep(200);
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
	message_type *data = new message_type[8];
   size_t i(0);
	
	data[i++] = 7 * sizeof(message_type);
	data[i++] = SET_COLOR;
   data[i++] = 0;
	data[i++] = (message_type)n->get_id();
	data[i++] = (message_type)r;
	data[i++] = (message_type)g;
	data[i++] = (message_type)b;
   data[i++] = 0; // intensity

   schedule_new_message(data);
}

void
sim_sched::schedule_new_message(message_type *data)
{
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

	no->pending.pop_list(ls);
	
	while(!no->tuple_pqueue.empty()) {
		pr = no->tuple_pqueue.min_value();
		
		if(pr.int_priority > no->timestamp)
			break;
		
		db::simple_tuple *stpl(no->tuple_pqueue.pop());
		ls.push_back(stpl);
	}

   while(!no->rtuple_pqueue.empty()) {
      pr = no->rtuple_pqueue.min_value();
      if(pr.int_priority > no->timestamp)
         break;
      
      db::simple_tuple *stpl(no->rtuple_pqueue.pop());
      ls.push_back(stpl);
   }
}

}
