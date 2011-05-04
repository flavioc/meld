
#include <iostream>
#include <boost/function.hpp>
#include <boost/thread/tss.hpp>

#include "process/process.hpp"
#include "vm/exec.hpp"
#include "process/machine.hpp"
#include "mem/thread.hpp"

using namespace db;
using namespace vm;
using namespace std;
using namespace utils;
using namespace boost;

namespace process {

static thread_specific_ptr<process> proc_ptr(NULL);

//#define DEBUG_ACTIVE 1

bool
process::all_buffers_emptied(void)
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(!buffered_work[i].empty())
         return false;
   }
   return true;
}

void
process::flush_this_queue(wqueue_free<work_unit>& q, process *other)
{
   assert(this != other);
   
   //printf("%d: flushing %d works to proc %d\n", get_id(), q.size(), other->get_id());
   
   other->make_active();

   other->queue_work.snap(q);
   q.clear();
}

void
process::flush_buffered(void)
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(i != get_id()) {
         wqueue_free<work_unit>& q(buffered_work[i]);
         if(!q.empty())
            flush_this_queue(q, state::MACHINE->get_process(i));
      }
   }
}

void
process::enqueue_other(process *other, node* node, const simple_tuple *stuple)
{
   static const size_t WORK_THRESHOLD(20);
   
   work_unit work = {node, stuple, false};
   wqueue_free<work_unit>& q(buffered_work[other->get_id()]);
   
   q.push(work);
   
   if(q.size() > WORK_THRESHOLD) {
      flush_this_queue(q, other);
   }
}

void
process::enqueue_work(node* node, const simple_tuple* stpl, const bool is_agg)
{
   work_unit work = {node, stpl, is_agg};
   
   make_active();

   queue_work.push(work);
}

bool
process::busy_wait(void)
{
   static const size_t COUNT_UP_TO(2);
    
   size_t cont(0);
   bool turned_inactive(false);
   
#ifdef COMPILE_MPI
   if(state::ROUTER->use_mpi()) {
      msg_buf.transmit();
      update_pending_messages();
      usleep(30 * 1000);
      fetch_work();
   }
#endif
   
   while(queue_work.empty()) {
      
      if(cont == 0 && !state::ROUTER->use_mpi())
         flush_buffered();
      
      // thread case
      if(!state::ROUTER->use_mpi() && state::MACHINE->finished())
         return false;

#ifdef COMPILE_MPI
      if(state::ROUTER->use_mpi())
         update_pending_messages();
#endif

      if(cont >= COUNT_UP_TO && !turned_inactive
#ifdef COMPILE_MPI
					&& msg_buf.all_received()
#endif
					) {
         make_inactive();
         turned_inactive = true;
      } else if(cont < COUNT_UP_TO || state::ROUTER->use_mpi())
         cont++;

#ifdef COMPILE_MPI
      if(state::ROUTER->use_mpi()) {
         if(turned_inactive)
            state::ROUTER->update_status(router::REMOTE_IDLE);
         
         update_remotes();
      
         if(turned_inactive && state::ROUTER->finished()) {
#ifdef DEBUG_REMOTE
            cout << "ITERATION ENDED for " << remote::self->get_rank() << endl;
#endif
            return false;
         }
      }
#endif
      
#ifdef COMPILE_MPI
      //cout << "Sleep for " << cont * 50 << " ms" << endl;
      usleep(cont * 50 * 1000);
      fetch_work();
#endif
   }
   
   assert(!queue_work.empty());
   
   return true;
}

void
process::make_inactive(void)
{
   if(process_state == PROCESS_ACTIVE) {
      mutex.lock();
      if(process_state == PROCESS_ACTIVE) {
         process_state = PROCESS_INACTIVE;
#ifdef DEBUG_ACTIVE
         printf("Inactive %d\n", id);
#endif
         state::MACHINE->process_is_inactive();
      }
      mutex.unlock();
   }
}

void
process::make_active(void)
{
   if(process_state == PROCESS_INACTIVE) {
      mutex::scoped_lock l(mutex);
      
      if(process_state == PROCESS_INACTIVE) {
         state::MACHINE->process_is_active();
         process_state = PROCESS_ACTIVE;
#ifdef DEBUG_ACTIVE
         printf("Active %d\n", id);
#endif
      }   
   }
}

bool
process::get_work(work_unit& work)
{
#ifdef COMPILE_MPI
   static const size_t ROUND_TRIP_FETCH_MPI(40);
   static const size_t ROUND_TRIP_UPDATE_MPI(40);
   static const size_t ROUND_TRIP_SEND_MPI(40);
   
   ++round_trip_fetch;
   ++round_trip_update;
   ++round_trip_send;
   
   if(round_trip_fetch == ROUND_TRIP_FETCH_MPI) {
      fetch_work();
      round_trip_fetch = 0;
   }
   
   if(round_trip_update == ROUND_TRIP_UPDATE_MPI) {
      update_pending_messages();
      round_trip_update = 0;
   }
   
   if(round_trip_send == ROUND_TRIP_SEND_MPI) {
      msg_buf.transmit();
      round_trip_send = 0;
   }
#endif
   
   if(queue_work.empty()) {
      if(!busy_wait())
         return false;

      make_active();
   }
   
   work = queue_work.pop();
   
   return true;
}

void
process::add_node(node *node)
{
   nodes.push_back(node);
   
   if(nodes_interval == NULL)
      nodes_interval = new interval<node::node_id>(node->get_id(), node->get_id());
   else
      nodes_interval->update(node->get_id());
}

void
process::do_tuple_add(node *node, vm::tuple *tuple, const ref_count count)
{
   const bool is_new(node->add_tuple(tuple, count));

   if(is_new) {
      // set vm state
      state.tuple = tuple;
      state.node = node;
      state.count = count;
      execute_bytecode(state.PROGRAM->get_bytecode(tuple->get_predicate_id()), state);
   } else
      delete tuple;
}

void
process::do_work(node *node, const simple_tuple *_stuple, const bool ignore_agg)
{
   auto_ptr<const simple_tuple> stuple(_stuple);
   vm::tuple *tuple = stuple->get_tuple();
   ref_count count = stuple->get_count();
   
   //cout << node->get_id() << " " << *stuple << endl;
   
   ++total_processed;
   
   if(count == 0)
      return;
   
   if(count > 0) {
      if(tuple->is_aggregate() && !ignore_agg)
         node->add_agg_tuple(tuple, count);
      else
         do_tuple_add(node, tuple, count);
   } else {
      count = -count;
      
      if(tuple->is_aggregate() && !ignore_agg) {
         node->remove_agg_tuple(tuple, count);
      } else {
         node::delete_info deleter(node->delete_tuple(tuple, count));
         
         if(deleter.to_delete()) { // to be removed
            state.tuple = tuple;
            state.node = node;
            state.count = -count;
            execute_bytecode(state.PROGRAM->get_bytecode(tuple->get_predicate_id()), state);
            deleter();
         } else
            delete tuple; // as in the positive case, nothing to do
      }
   }
}

void
process::update_remotes(void)
{
   static size_t UPDATE_REMOTES(0);
   
#ifdef COMPILE_MPI
   if(state::ROUTER->use_mpi()) {
      state::ROUTER->fetch_updates();
      ++UPDATE_REMOTES;
      
      //printf("%d\n", UPDATE_REMOTES);
   }
#endif
}

void
process::fetch_work(void)
{
#ifdef COMPILE_MPI
   if(state::ROUTER->use_mpi()) {
      message_set *ms;
      
      while((ms = state::ROUTER->recv_attempt(get_id())) != NULL) {
         assert(!ms->empty());
         
         for(list_messages::const_iterator it(ms->begin()); it != ms->end(); ++it) {
            message *msg(*it);
            
            enqueue_work(state::DATABASE->find_node(msg->id), msg->data);
            
            delete msg;
         }
         
#ifdef DEBUG_REMOTE
         cout << "Received " << ms->size() << " works" << endl;
#endif
         
         delete ms;
      }
      
      assert(ms == NULL);
   }
#endif
}

void
process::enqueue_remote(remote *rem, const process_id proc, message *msg)
{
#ifdef COMPILE_MPI
   msg_buf.insert(get_id(), rem, proc, msg);
#endif
}

void
process::update_pending_messages(void)
{
#ifdef COMPILE_MPI
   msg_buf.update_received();
#endif
}

void
process::generate_aggs(void)
{
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end();
      ++it)
   {
      node *no(*it);
      simple_tuple_list ls(no->generate_aggs());

      for(simple_tuple_list::iterator it2(ls.begin());
         it2 != ls.end();
         ++it2)
      {
         //cout << no->get_id() << " GENERATE " << **it2 << endl;
         enqueue_work(no, *it2, true);
      }
   }
}

void
process::do_loop(void)
{
   work_unit work;

   while(true) {
      while(get_work(work))
         do_work(work.work_node, work.work_tpl, work.agg);
   
      assert(queue_work.empty());
      assert(process_state == PROCESS_INACTIVE);
#ifdef COMPILE_MPI
      assert(msg_buf.empty());
      assert(msg_buf.all_received());
#endif
      
#ifdef COMPILE_MPI
      if(state::ROUTER->use_mpi()) {
         state::ROUTER->synchronize();
         
         generate_aggs();
         num_aggs++;
         
         if(process_state == PROCESS_ACTIVE)
            state::ROUTER->update_status(router::REMOTE_ACTIVE);
         
         state::ROUTER->synchronize();
         
         update_remotes();
         
         //state::ROUTER->synchronize();
         
         if(state::ROUTER->finished())
            return;
         
      } else
#endif
      {  
         state::MACHINE->wait_aggregates();
         
         //printf("%d - HERE %d\n", num_aggs, id);
         generate_aggs();
         num_aggs++;
         
         //printf("%d - HERE END %d\n", num_aggs, id);
         state::MACHINE->wait_aggregates();

         if(state::MACHINE->finished()) {
            //printf("Machine finished %d\n", id);
            return;
         }
         
         state::MACHINE->wait_aggregates();
      }
   }
}

void
process::loop(void)
{
   // start process pool
   mem::create_pool(get_id());
   proc_ptr.reset(this);
   
   // init buffered queues
   buffered_work.resize(state::NUM_THREADS);
   
   // enqueue init tuple
   predicate *init_pred(state.PROGRAM->get_init_predicate());
   
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end();
      ++it)
   {
      node *cur_node(*it);
      
      enqueue_work(cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
   }
   
   do_loop();

   assert(queue_work.empty());
   assert(all_buffers_emptied());
   //printf("Processed: %d Aggs %d\n", total_processed, num_aggs);
}

void
process::start(void)
{
   if(get_id() == 0) {
      thread = new boost::thread();
      loop();
   } else
      thread = new boost::thread(bind(&process::loop, this));
}

process::process(const process_id& _id):
   nodes_interval(NULL),
   thread(NULL),
   id(_id),
   process_state(PROCESS_ACTIVE),
   state(this),
   total_processed(0),
   num_aggs(0)
#ifdef COMPILE_MPI
   ,round_trip_fetch(0),
   round_trip_update(0),
   round_trip_send(0)
#endif
{
}

process::~process(void)
{
   delete nodes_interval;
   delete thread;
}

void
process::print(ostream& cout) const
{
   cout << *nodes_interval;
}

ostream& operator<<(ostream& cout, const process& proc)
{
   proc.print(cout);
   return cout;
}

}
