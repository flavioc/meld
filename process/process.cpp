
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
   
program *process::PROGRAM = NULL;
database *process::DATABASE = NULL;
mutex process::remote_mutex;
static thread_specific_ptr<process> proc_ptr(NULL);

#define MAKE_IT_STOP 1
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
   
#ifdef MAKE_IT_STOP
   other->make_active();
#endif

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
      //printf("HERE\n");
      flush_this_queue(q, other);
   }
   //other->enqueue_work(node, stuple);
}

void
process::enqueue_work(node* node, const simple_tuple* stpl, const bool is_agg)
{
   work_unit work = {node, stpl, is_agg};
   
#ifdef MAKE_IT_STOP
   make_active();
#endif
   
   queue_work.push(work);
}

bool
process::busy_wait(void)
{
   static const unsigned int COUNT_UP_TO(2);
    
   unsigned int cont(0);
   
   while(queue_work.empty()) {
      
      if(cont == 0)
         flush_buffered();
         
#ifdef MAKE_IT_STOP
      if(state::MACHINE->finished())
         return false;
#endif
         
      if(cont == COUNT_UP_TO)
         make_inactive();
      else if(cont < COUNT_UP_TO)
         cont++;

#ifdef COMPILE_MPI
      if(get_id() == 0) {
         update_remotes();
      
         if(state::MACHINE->finished() && state::ROUTER->finished()) {
         
            // wait a bit before making a good decision
            boost::this_thread::sleep(boost::posix_time::millisec(25));
         
            fetch_work();
         
            if(!queue.empty())
               return true;
            
            update_remotes();
            
            if(state::MACHINE->finished() && state::ROUTER->finished()) {
               state::MACHINE->mark_finished();
               printf("ENDING HERE %d\n", remote::self->get_rank());
               return false;
            }
         }
      } else
#endif
      
#ifdef COMPILE_MPI   
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
      mutex.lock();
      if(process_state == PROCESS_INACTIVE) {
         state::MACHINE->process_is_active();
         process_state = PROCESS_ACTIVE;
#ifdef DEBUG_ACTIVE
         printf("Active %d\n", id);
#endif
      }
      mutex.unlock();
   }
}

bool
process::get_work(work_unit& work)
{
#ifdef COMPILE_MPI
   fetch_work();
#endif
   
   if(queue_work.empty()) {
      if(!busy_wait())
         return false;

#ifdef MAKE_IT_STOP
      make_active();
#endif
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
process::fetch_work(void)
{
#ifdef COMPILE_MPI
   if(state::ROUTER->use_mpi()) {
      message *msg;
      
      while((msg = state::ROUTER->recv_attempt(get_id()))) {
         enqueue_work(state::DATABASE->find_node(msg->id), msg->data);
         
         delete msg;
      }
   }
#endif
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
#ifdef COMPILE_MPI
   if(state::ROUTER->use_mpi() && get_id() == 0) {
      remote_mutex.lock();
      state::ROUTER->fetch_updates();
      remote_mutex.unlock();
   }
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
   
#ifdef COMPILE_MPI
      if(state::ROUTER->use_mpi()) {
         ended = true;

         if(get_id() == 0) {
            while(!state::MACHINE->all_ended())
               {}
         
            state::ROUTER->finish();
         }
         
         return;
      } else
#endif
      {
         assert(queue_work.empty());
         assert(process_state == PROCESS_INACTIVE);
         
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

process::process(const process_id& _id, const size_t num_procs):
   nodes_interval(NULL), thread(NULL), id(_id),
   process_state(PROCESS_ACTIVE),
   ended(false),
   agg_checked(false),
   state(this),
   total_processed(0),
   num_aggs(0)
{
   buffered_work.resize(num_procs);
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
