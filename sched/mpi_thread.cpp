
#include "sched/mpi_thread.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"
#include "utils/utils.hpp"

using namespace std;
using namespace vm;
using namespace process;
using namespace db;
using namespace boost;
using namespace utils;

namespace sched
{
   
volatile static bool iteration_finished;
static size_t round_trip_fetch(0);
static tokenizer *token;
static mutex tok_mutex;
static size_t step_fetch(MPI_DEFAULT_ROUND_TRIP_FETCH);

void
mpi_thread::assert_end(void) const
{
   dynamic_local::assert_end();
   assert(iteration_finished);
   assert(msg_buf.empty());
}

void
mpi_thread::assert_end_iteration(void) const
{
   dynamic_local::assert_end_iteration();
   assert(iteration_finished);
   assert(msg_buf.empty());
}

void
mpi_thread::update_pending_messages(void)
{
   msg_buf.update_received();
}

void
mpi_thread::transmit_messages(void)
{
   size_t total(msg_buf.transmit());
   
   {
      mutex::scoped_lock lock(tok_mutex);
      
      token->transmitted(total);
   }
}

void
mpi_thread::change_node(thread_node *node, dynamic_local *_asker)
{
   mpi_thread *asker((mpi_thread*)_asker);
   
   assert(node != current_node);
   assert(node->get_owner() == this);
   
   remove_node(node);
   asker->add_node(node);
   
   {
      mutex::scoped_lock lock(node->mtx);
      node->set_owner((static_local*)asker);
      assert(node->in_queue());
      assert(node->get_owner() == asker);
   }
   
   asker->add_to_queue(node);
}

bool
mpi_thread::busy_wait(void)
{
   volatile bool turned_inactive(false);
   size_t asked_many(0);
   
   transmit_messages();
   if(leader_thread())
      fetch_work();
   
   while(!has_work()) {
      
      update_pending_messages();
      
      if(state::NUM_THREADS > 1 && asked_many < MAX_ASK_STEAL) {
         mpi_thread *target((mpi_thread*)select_steal_target());
         
         if(target->is_active()) {
            target->request_work_to(this);
            ++asked_many;
         }
      }
      
      if(!turned_inactive && !has_work()) {
         assert(!turned_inactive);
         mutex::scoped_lock l(mutex);
         if(!has_work()) {
            if(is_active()) {
               set_inactive();
            }
            turned_inactive = true;
            if(!leader_thread() && iteration_finished)
               return false;
         }
      }
      
      if(leader_thread() && all_threads_finished()) {
         mutex::scoped_lock lock(tok_mutex);
         if(!token->busy_loop_token(all_threads_finished())) {
            assert(all_threads_finished());
            assert(turned_inactive);
            iteration_finished = true;
            return false;
         }
      }
      
      if(!leader_thread() && turned_inactive && is_inactive() && iteration_finished) {
         assert(!leader_thread()); // leader thread does not finish here
         assert(is_inactive());
         assert(turned_inactive);
         assert(iteration_finished);
         assert(all_threads_finished());
         return false;
      }
      
      if(leader_thread())
         fetch_work();
   }
   
   set_active_if_inactive();
   
   assert(is_active());
   assert(has_work());
   
   return true;
}

void
mpi_thread::fetch_work(void)
{
   assert(leader_thread());
   
   if(!state::ROUTER->use_mpi())
      return;
   
   message_set *ms;
   bool any(false);
      
   while((ms = state::ROUTER->recv_attempt(0)) != NULL) {
      assert(!ms->empty());
      
      for(list_messages::const_iterator it(ms->begin()); it != ms->end(); ++it) {
         message *msg(*it);
         thread_node *node((thread_node*)state::DATABASE->find_node(msg->id));
         simple_tuple *stpl(msg->data);
         mutex::scoped_lock lock(node->mtx);
         
         if(node->get_owner() == this) {
            node->add_work(stpl, false);
            if(!node->in_queue()) {
               node->set_in_queue(true);
               this->add_to_queue(node);
            }
            assert(node->in_queue());
         } else {
            mpi_thread *owner = (mpi_thread*)node->get_owner();
            
            node->add_work(stpl, false);
            
            if(!node->in_queue()) {
               node->set_in_queue(true);
               owner->add_to_queue(node);
            }
            
            assert(owner != NULL);
            
            if(owner->is_inactive()) {
               mutex::scoped_lock lock(owner->mutex);
               if(owner->is_inactive() && owner->has_work())
                  owner->set_active();
            }
         }
          
         delete msg;
      }
         
#ifdef DEBUG_REMOTE
      cout << "Received " << ms->size() << " works" << endl;
#endif

      {
         mutex::scoped_lock lock(tok_mutex);
         token->one_message_received();
      }
      
      delete ms;
      any = true;
   }
   
   if(any && step_fetch > MPI_MIN_ROUND_TRIP_FETCH)
		step_fetch -= MPI_DECREASE_ROUND_TRIP_FETCH;
	if(!any && step_fetch < MPI_MAX_ROUND_TRIP_FETCH)
		step_fetch += MPI_INCREASE_ROUND_TRIP_FETCH;
      
   assert(ms == NULL);
}

void
mpi_thread::new_work_remote(remote *rem, const node::node_id, message *msg)
{
   if(msg_buf.insert(rem, 0, msg)) {
      mutex::scoped_lock lock(tok_mutex);
      token->transmitted(1);
   }
}

bool
mpi_thread::get_work(work_unit& work)
{  
   ++round_trip_update;
   ++round_trip_send;

   if(round_trip_update == MPI_ROUND_TRIP_UPDATE) {
      update_pending_messages();
      round_trip_update = 0;
   }

   if(round_trip_send == MPI_ROUND_TRIP_SEND) {
      transmit_messages();
      round_trip_send = 0;
   }
   
   if(leader_thread()) {
      ++round_trip_fetch;
      
      if(round_trip_fetch == step_fetch) {
         fetch_work();
         round_trip_fetch = 0;
      }
   }
   
   return dynamic_local::get_work(work);
}

bool
mpi_thread::terminate_iteration(void)
{
   // this is needed since one thread can reach set_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   threads_synchronize();
   
   if(leader_thread())
      token->token_terminate_iteration();

   assert(iteration_finished);
   assert(is_inactive());

   generate_aggs();

   if(has_work())
      set_active();

   // again, needed since we must wait if any thread
   // is set to active in the previous if
   threads_synchronize();
   
   if(leader_thread()) {
      const bool more_work = state::ROUTER->reduce_continue(!all_threads_finished());
      iteration_finished = !more_work;
   }
   
   // threads must wait for the final answer between processes
   threads_synchronize();

   return !iteration_finished;
}
   
vector<sched::base*>&
mpi_thread::start(const size_t num_threads)
{
   init_barriers(num_threads);
   token = new sched::tokenizer();
   
   iteration_finished = false;
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new mpi_thread(i));
   
   return ALL_THREADS;
}

}
