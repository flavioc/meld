
#include "sched/local/threads_dynamic.hpp"
#include "vm/state.hpp"
#include "db/database.hpp"
#include "process/remote.hpp"
#include "utils/utils.hpp"
#include "sched/common.hpp"

using namespace vm;
using namespace utils;
using namespace db;
using namespace boost;
using namespace std;
using namespace process;

namespace sched
{

void
dynamic_local::assert_end(void) const
{
   static_local::assert_end();
   
#ifdef MARK_OWNED_NODES
   for(node_set::iterator it(nodes->begin()); it != nodes->end(); ++it) {
      thread_node *node((thread_node*)*it);
      node->assert_end();
   }
#else
   assert_static_nodes_end(id);
#endif
}

void
dynamic_local::assert_end_iteration(void) const
{
   static_local::assert_end_iteration();
   
#ifdef MARK_OWNED_NODES
   for(node_set::iterator it(nodes->begin()); it != nodes->end(); ++it) {
      thread_node *node((thread_node*)*it);
      node->assert_end_iteration();
   }
#else
	 assert_static_nodes_end_iteration(id);
#endif
}

#ifdef MARK_OWNED_NODES
void
dynamic_local::add_node(node *node)
{
   assert(nodes != NULL);
   assert(node != NULL);
   
   spinlock::scoped_lock l(*nodes_mutex);

   nodes->insert(node);
}

void
dynamic_local::remove_node(node *node)
{
   assert(nodes != NULL);
   assert(node != NULL);
   
   spinlock::scoped_lock l(*nodes_mutex);

   nodes->erase(node);
}
#endif

void
dynamic_local::end(void)
{
   // cleanup the steal set
   steal.clear();
}

#ifndef MARK_OWNED_NODES
void
dynamic_local::new_agg(work& new_work)
{
   thread_node *to(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
   
   to->add_work(node_new_work);
   
   if(!to->in_queue()) {
      // note the 'get_owner'
      to->get_owner()->add_to_queue(to);
      to->set_in_queue(true);
   }
}
#endif

dynamic_local*
dynamic_local::select_steal_target(void) const
{
   size_t idx(random_unsigned(state::NUM_THREADS));
   
   while(ALL_THREADS[idx] == this)
      idx = random_unsigned(state::NUM_THREADS);
   
   return dynamic_cast<dynamic_local*>(ALL_THREADS[idx]);
}

void
dynamic_local::request_work_to(dynamic_local *asker)
{
#ifdef INSTRUMENTATION
   steal_requests++;
#endif
   steal.push(asker);
}

static inline size_t
find_max_steal_attempts(void)
{
   switch(state::NUM_THREADS) {
      case 2: return 2;
      case 3: return 4;
      default: return state::NUM_THREADS + state::NUM_THREADS/2;
   }
}

static inline size_t
find_max_ask_steal_round(void)
{
   switch(state::NUM_THREADS) {
      case 2: return 1;
      case 3: return 2;
      case 4: return 2;
      default: return (state::NUM_THREADS-1)/2;
   }
}

void
dynamic_local::steal_nodes(size_t& asked_this_round)
{
   if(state::NUM_THREADS == 1)
      return;
   
   if(next_steal_cycle > 0) {
      next_steal_cycle--;
      if(next_steal_cycle > 0)
         return;
   }
      
   if(is_active())
      return;
   
   if(asked_this_round > find_max_ask_steal_round())
      return;
   
   dynamic_local *selected_target(NULL);
   
   for(size_t attempts(0); attempts < find_max_steal_attempts(); ++attempts) {
      dynamic_local *target(select_steal_target());
      
      if(target->is_active()) {
         selected_target = target;
         break;
      }
   }
   
   if(selected_target == NULL) {
      next_steal_cycle += DELAY_STEAL_CYCLE;
      // I should stop asking
      return;
   }
   
   selected_target->request_work_to(this);
   ++asked_this_round;
}

bool
dynamic_local::busy_wait(void)
{
   size_t asked_this_round(0);
   
   while(!has_work()) {
      
      steal_nodes(asked_this_round);
      
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   }
   
   set_active_if_inactive();
   
   assert(is_active());
   assert(has_work());
   
   return true;
}

void
dynamic_local::change_node(thread_node *node, dynamic_local *asker)
{
   assert(node != current_node);
   assert(node->get_owner() == this);
   
   // change ownership
   
#ifdef MARK_OWNED_NODES
   remove_node(node);
   asker->add_node(node);
#endif
   
   {
      spinlock::scoped_lock l(node->spin);
      node->set_owner(dynamic_cast<static_local*>(asker));
      assert(node->in_queue());
      assert(node->get_owner() == asker);
   }
	 asker->add_to_queue(node);

#ifdef INSTRUMENTATION
   asker->stealed_nodes++;
#endif
}

void
dynamic_local::handle_stealing(void)
{
   assert(state::NUM_THREADS > 1);
   
   while(!steal.empty() && has_work()) {
      assert(!steal.empty());
      assert(has_work());
      
      dynamic_local *asker(dynamic_cast<dynamic_local*>(steal.pop()));
      
      assert(asker != NULL);
      
      //cout << "Answering request of " << (int)asker->get_id() << endl;
      size_t total_sent(0);
      
      while(has_work() && total_sent < MAX_SEND_PER_TIME) {
         thread_node *node(queue_nodes.pop());

         assert(node != NULL);
         
         change_node(node, asker);
         
         ++total_sent;
      }
      
      assert(total_sent > 0);
   
      MAKE_OTHER_ACTIVE(asker);
   }
}

bool
dynamic_local::get_work(work& new_work)
{
   if(state::NUM_THREADS > 1)
      handle_stealing();
      
   return static_local::get_work(new_work);
}
   
void
dynamic_local::init(const size_t)
{
#ifdef MARK_OWNED_NODES
   nodes_mutex = new spinlock();
   nodes = new node_set();
#endif

   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
     
      cur_node->set_owner(this);
      
#ifdef MARK_OWNED_NODES
      nodes->insert(cur_node);
#endif   
   
      init_node(cur_node);
      
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

void
dynamic_local::generate_aggs(void)
{
#ifdef MARK_OWNED_NODES
   for(node_set::iterator it(nodes->begin()); it != nodes->end(); ++it) {
      node *node(*it);
      assert(((thread_node*)node)->get_owner() == this);
      node_iteration(node);
   }
#else
   iterate_static_nodes(id);
#endif
}

void
dynamic_local::write_slice(stat::slice& sl) const
{
#ifdef INSTRUMENTATION
   static_local::write_slice(sl);
   sl.stealed_nodes = stealed_nodes;
   stealed_nodes = 0;
   sl.steal_requests = steal_requests;
   steal_requests = 0;
#else
   (void)sl;
#endif
}

dynamic_local*
dynamic_local::find_scheduler(const node *n)
{
   const thread_node *tn(dynamic_cast<const thread_node*>(n));
   
   if(tn->get_owner() == this)
      return this;
   else
      return NULL;
}

dynamic_local::dynamic_local(const process_id id):
   static_local(id),
#ifdef MARK_OWNED_NODES
   nodes(NULL),
   nodes_mutex(NULL),
#endif
	next_steal_cycle(0)
#ifdef INSTRUMENTATION
   , stealed_nodes(0)
   , steal_requests(0)
#endif
{
}
   
dynamic_local::~dynamic_local(void)
{
#ifdef MARK_OWNED_NODES
   assert(nodes != NULL);
   assert(nodes_mutex != NULL);
   
   delete nodes;
   delete nodes_mutex;
#endif
}

vector<sched::base*>&
dynamic_local::start(const size_t num_threads)
{
   init_barriers(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new dynamic_local(i));
      
   return ALL_THREADS;
}
   
}
