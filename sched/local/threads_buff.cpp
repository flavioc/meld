
#include "sched/local/threads_buff.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "sched/thread/assert.hpp"
#include "vm/state.hpp"
#include "sched/common.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

#define DEFAULT_ROUND_TRIP_RECEIVE MPI_DEFAULT_ROUND_TRIP_SEND

namespace sched
{

void
static_buff::assert_end(void) const
{
   assert(!has_work());
   assert(!has_incoming_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end(id);
}

void
static_buff::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(!has_incoming_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end_iteration(id);
}

void
static_buff::new_agg(work& new_work)
{
   unsafe_static_node *to(dynamic_cast<unsafe_static_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
   to->add_work(node_new_work);
   
   if(!to->in_queue()) {
      add_to_queue(to);
      to->set_in_queue(true);
   }
}

void
static_buff::new_work(const node *, work& new_work)
{
   unsafe_static_node *to(dynamic_cast<unsafe_static_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
   
   to->add_work(node_new_work);
   
   if(!to->in_queue()) {
      add_to_queue(to);
      to->set_in_queue(true);
   }
   
   assert(to->in_queue());
}

void
static_buff::new_work_other(sched::base *target, work& new_work)
{
   assert(is_active());
   assert_thread_push_work();
   
   map_buffer::iterator it(tuples_buf.find(target));
   
   if(it == tuples_buf.end()) {
      pair<map_buffer::iterator, bool> res(
         tuples_buf.insert(pair<sched::base*, queue_buffer>(target, queue_buffer())));
      
      it = res.first;
   }
   
   queue_buffer& q(it->second);
   
   q.push(new_work);
#ifdef INSTRUMENTATION
   sent_facts++;
#endif
}

void
static_buff::process_incoming(void)
{
   if(incoming.empty())
      return;
      
   do {
      work w(incoming.pop());
      unsafe_static_node *node(dynamic_cast<unsafe_static_node*>(w.get_node()));
      
      node_work node_new_work(w);
      node->add_work(node_new_work);
      
      if(!node->in_queue()) {
         node->set_in_queue(true);
		   add_to_queue(node);
		}
   } while (!incoming.empty());
}

bool
static_buff::busy_wait(void)
{
   flush_buffer();
   process_incoming();
   ins_idle;
   
   while(!has_any_work()) {
      if(is_active() && !has_any_work()) {
         spinlock::scoped_lock l(lock);
         if(!has_any_work()) {
            if(is_active())
               set_inactive();
         } else
            break;
      }
      
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   }
   
   process_incoming();
   
   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   ins_active;
   assert(is_active());
   assert(has_work());
   
   return true;
}

void
static_buff::flush_buffer(void)
{
   for(map_buffer::iterator it(tuples_buf.begin()); it != tuples_buf.end(); ++it) {
      static_buff *target((static_buff*)it->first);
      queue_buffer& q(it->second);
      
      if(!q.empty()) {
         target->incoming.snap(q);
         
         q.clear();
         
         {
            spinlock::scoped_lock l(target->lock);
         
            if(target->is_inactive() && target->has_any_work())
            {
               target->set_active();
               assert(target->is_active());
            }
         }
      }
   }
}

void
static_buff::generate_aggs(void)
{
   iterate_static_nodes(id);
}

bool
static_buff::set_next_node(void)
{
   if(current_node != NULL) {
      if(current_node->has_work())
         return true;
      current_node->set_in_queue(false);
      current_node = NULL;
   }
   
   while (current_node == NULL) {   
      if(!has_work()) {
         if(!busy_wait())
            return false;
      }
      
      assert(has_work());
      
      current_node = queue_nodes.pop();
      
      assert(current_node->in_queue());
      assert(current_node != NULL);
      assert(current_node->has_work());
   }
   
   ins_active;
   
   assert(current_node != NULL);
   
   return true;
}

bool
static_buff::get_work(work& new_work)
{
   round_trip_send++;
   
   if(round_trip_send == step_send) {
      round_trip_send = 0;
      flush_buffer();
   }
   
   round_trip_receive++;
   
   if(round_trip_receive == step_receive) {
      round_trip_receive = 0;
      process_incoming();
   }
   
   if(!set_next_node())
      return false;
   
   set_active_if_inactive();
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->has_work());
   
   node_work unit(current_node->get_work());
   
   new_work.copy_from_node(current_node, unit);
   
   assert(new_work.get_node() == current_node);
   
   assert_thread_pop_work();
   
   return true;
}

void
static_buff::init(const size_t)
{
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      unsafe_static_node *cur_node(dynamic_cast<unsafe_static_node*>(it->second));
      
      init_node(cur_node);
      
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

bool
static_buff::terminate_iteration(void)
{
   START_ROUND();
   
   if(has_work())
      set_active();
   
   END_ROUND(
      more_work = num_active() > 0;
   );
}

static_buff*
static_buff::find_scheduler(const node *n)
{
   return (static_buff*)ALL_THREADS[remote::self->find_proc_owner(n->get_id())];
}

void
static_buff::write_slice(stat::slice& sl) const
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   sl.work_queue = queue_nodes.size();
#else
   (void)sl;
#endif
}

static_buff::static_buff(const vm::process_id _id):
   base(_id),
   current_node(NULL),
   step_receive(DEFAULT_ROUND_TRIP_RECEIVE),
   round_trip_receive(0),
   step_send(MPI_DEFAULT_ROUND_TRIP_SEND),
   round_trip_send(0)
{
}

static_buff::~static_buff(void)
{
}
   
vector<sched::base*>&
static_buff::start(const size_t num_threads)
{
   init_barriers(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new static_buff(i));
      
   return ALL_THREADS;
}
   
}
