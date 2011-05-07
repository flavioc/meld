#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/stealer.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "vm/state.hpp"
#include "utils/utils.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

namespace sched
{

static vector<stealer*> others;
static barrier *thread_barrier(NULL);
static size_t threads_active(0);

static inline void
make_thread_active(void)
{
   __sync_fetch_and_add(&threads_active, 1);
}

static inline void
make_thread_inactive(void)
{
   __sync_fetch_and_sub(&threads_active, 1);
}

static inline const bool
are_threads_finished(void)
{
   return threads_active == 0;
}

static inline void
threads_synchronize(void)
{
   thread_barrier->wait();
}

bool
stealer::all_buffers_emptied(void) const
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(!buffered_work[i].empty())
         return false;
   }
   return true;
}

void
stealer::assert_end(void) const
{
   assert(queue_work.empty());
   assert(process_state == PROCESS_INACTIVE);
   assert(all_buffers_emptied());
   assert(threads_active == 0);
}

void
stealer::assert_end_iteration(void) const
{
   assert(queue_work.empty());
   assert(process_state == PROCESS_INACTIVE);
   assert(all_buffers_emptied());
   assert(threads_active == 0);
}

void
stealer::make_active(void)
{
   if(process_state == PROCESS_INACTIVE) {
      mutex::scoped_lock l(mutex);
      
      if(process_state == PROCESS_INACTIVE) {
         make_thread_active();
         process_state = PROCESS_ACTIVE;
#ifdef DEBUG_ACTIVE
         cout << "Active " << id << endl;
#endif
      }
   }
}

void
stealer::make_inactive(void)
{
   if(process_state == PROCESS_ACTIVE) {
      mutex::scoped_lock l(mutex);

      if(process_state == PROCESS_ACTIVE) {
         make_thread_inactive();
         process_state = PROCESS_INACTIVE;
#ifdef DEBUG_ACTIVE
         cout << "Inactive: " << id << endl;
#endif
      }
   }
}

void
stealer::flush_this_queue(wqueue_free<work_unit>& q, stealer *other)
{
   assert(this != other);
   
   other->make_active();
   other->queue_work.snap(q);
   
   q.clear();
}

void
stealer::new_work(node *node, const simple_tuple *tpl, const bool is_agg)
{
   work_unit work = {node, tpl, is_agg};

   queue_work.push(work);
   make_active();
}

void
stealer::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   static const size_t WORK_THRESHOLD(20);
   
   stealer *other((stealer*)scheduler);
   work_unit work = {node, stuple, false};
   wqueue_free<work_unit>& q(buffered_work[other->id]);
   
   q.push(work);
   
   if(q.size() > WORK_THRESHOLD)
      flush_this_queue(q, other);
}

void
stealer::new_work_remote(remote *, const vm::process_id, message *)
{
   assert(0);
}

void
stealer::generate_aggs(void)
{
}

void
stealer::flush_buffered(void)
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(i != id) {
         wqueue_free<work_unit>& q(buffered_work[i]);
         if(!q.empty())
            flush_this_queue(q, others[i]);
      }
   }
}

stealer*
stealer::select_steal_target(void) const
{
   size_t idx(random_unsigned(others.size()));
   
   while(others[idx] == this)
      idx = random_unsigned(others.size());
   
   return others[idx];
}

bool
stealer::busy_wait(void)
{
   static const size_t COUNT_UP_TO(100);

   if(state::NUM_THREADS == 1) {
      make_inactive();
      return false;
   }
   
   size_t cont(0);
   bool turned_inactive(false);
   
   flush_buffered();
   
   while(queue_work.empty()) {
      
      if(are_threads_finished())
         return false;

      if(cont == 20 && !turned_inactive) {
         make_inactive();
         turned_inactive = true;
      } else {
         cont++;
         
         if(cont == 10) {
            stealer *target(select_steal_target());
            
            if(!target->queue_work.empty()) {
               
               static const size_t HOW_MANY(3);
               queue_node<work_unit> *stolen(target->queue_work.steal(HOW_MANY));
               
               if(stolen) {
                  queue_node<work_unit> *coiso(stolen);
                  
                  while(coiso) {
                     work_unit& wu(coiso->data);
                     thread_node *node((thread_node*)wu.work_node);
                     node->set_owner(this);
                     
                     coiso = coiso->next;
                  }
                  
                  queue_work.append(stolen);
               }
            }
         }
         
         if(cont == COUNT_UP_TO)
            cont = 0;
      }
   }
   
   assert(!queue_work.empty());
   
   return true;
}

bool
stealer::terminate_iteration(void)
{
   threads_synchronize();
   
   generate_aggs();
   
   threads_synchronize();
   
   if(are_threads_finished())
      return false;
   
   threads_synchronize();
   
   return true;
}

void
stealer::finish_work(const work_unit& work)
{
   thread_node *node((thread_node*)work.work_node);
   
   assert(node->get_worker() == this);
   
   node->worker = NULL;
}

bool
stealer::get_work(work_unit& work)
{
   while(true) {
      if(queue_work.empty())
         if(!busy_wait())
            return false;
      
      make_active();
      
      if(!queue_work.pop_safe(work))
         continue;
      
      thread_node *node((thread_node*)work.work_node);

      if(__sync_bool_compare_and_swap(&node->worker, NULL, this)) {
         return true;
      } else {
         queue_work.push(work);
         continue;
      }
   }
}

void
stealer::end(void)
{
   delete nodes_mutex;
}

void
stealer::add_node(node *node)
{
   mutex::scoped_lock l(*nodes_mutex);
   
   nodes->insert(node);
}

void
stealer::remove_node(node *node)
{
   mutex::scoped_lock l(*nodes_mutex);
   
   nodes->erase(node);
}

void
stealer::init(const size_t num_threads)
{
   // init buffered queues
   buffered_work.resize(num_threads);
   
   nodes_mutex = new boost::mutex();
   nodes = new node_set();
   
   predicate *init_pred(state::PROGRAM->get_init_predicate());
   
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
      cur_node->set_owner(this);
      
      new_work(cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
      
      nodes->insert(cur_node);
   }
   
   threads_synchronize();
}

stealer*
stealer::find_scheduler(const node::node_id id)
{
   thread_node* node((thread_node*)state::DATABASE->find_node(id));
   
   //printf("FIND HERE!\n");
   assert(node != NULL);
   assert(node->get_owner() != NULL);
   
   return node->get_owner();
}

stealer::stealer(const vm::process_id _id):
   base(_id),
   process_state(PROCESS_ACTIVE),
   nodes(NULL)
{
}

stealer::~stealer(void)
{
   delete nodes;
}
   
vector<stealer*>&
stealer::start(const size_t num_threads)
{
   thread_barrier = new barrier(num_threads);
   others.resize(num_threads);
   threads_active = num_threads;
   
   for(process_id i(0); i < num_threads; ++i)
      others[i] = new stealer(i);
      
   return others;
}
   
}