
#include "sched/local/threads_dynamic.hpp"
#include "vm/state.hpp"
#include "db/database.hpp"
#include "process/remote.hpp"
#include "utils/utils.hpp"

using namespace vm;
using namespace utils;
using namespace db;
using namespace boost;
using namespace std;
using namespace process;

namespace sched
{
   
void
dynamic_local::add_node(node *node)
{
   assert(nodes != NULL);
   assert(node != NULL);
   
   mutex::scoped_lock l(*nodes_mutex);

   nodes->insert(node);
}

void
dynamic_local::remove_node(node *node)
{
   assert(nodes != NULL);
   assert(node != NULL);
   
   mutex::scoped_lock l(*nodes_mutex);

   nodes->erase(node);
}

void
dynamic_local::end(void)
{
   // cleanup the steal set
   steal.clear();
}

dynamic_local*
dynamic_local::select_steal_target(void) const
{
   size_t idx(random_unsigned(ALL_THREADS.size()));
   
   while(ALL_THREADS[idx] == this)
      idx = random_unsigned(ALL_THREADS.size());
   
   return (dynamic_local*)ALL_THREADS[idx];
}

void
dynamic_local::request_work_to(dynamic_local *asker)
{
   steal.push(asker);
}

bool
dynamic_local::busy_wait(void)
{
   bool turned_inactive(false);
   size_t asked_many(0);
   
   while(!has_work()) {
      
      if(state::NUM_THREADS > 1 && asked_many < MAX_ASK_STEAL) {
         dynamic_local *target(select_steal_target());
         
         if(target->is_active()) {
            target->request_work_to(this);
            ++asked_many;
         }
      }
      
      if(!turned_inactive && !has_work()) {
         mutex::scoped_lock l(mutex);
         if(!has_work()) {
            if(is_active())
               set_inactive();
            turned_inactive = true;
            if(all_threads_finished())
               return false;
         }
      }
      
      if(turned_inactive && is_inactive() && all_threads_finished()) {
         assert(is_inactive());
         assert(turned_inactive);
         return false;
      }
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
   
   node->set_owner((static_local*)asker);
   remove_node(node);
   asker->add_node(node);
   
   assert(node->in_queue());
   assert(node->get_owner() == asker);
   
   asker->add_to_queue(node);
}

void
dynamic_local::handle_stealing(void)
{
   assert(state::NUM_THREADS > 1);
   
   while(!steal.empty() && has_work()) {
      assert(!steal.empty());
      assert(has_work());
      
      dynamic_local *asker((dynamic_local*)steal.pop());
      
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
      
      if(asker->is_inactive()) {
         mutex::scoped_lock lock(asker->mutex);
         if(asker->is_inactive() && asker->has_work())
            asker->set_active();
      }
   }
}

bool
dynamic_local::get_work(work_unit& work)
{
   if(state::NUM_THREADS > 1)
      handle_stealing();
   return static_local::get_work(work);
}
   
void
dynamic_local::init(const size_t num_threads)
{
   nodes_mutex = new boost::mutex();
   nodes = new node_set();
   
   predicate *init_pred(state::PROGRAM->get_init_predicate());
   
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
     
      cur_node->set_owner(this);
      nodes->insert(cur_node);
      
      new_work(NULL, cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
      
      assert(cur_node->in_queue());
      assert(!cur_node->no_more_work());
   }
   
   threads_synchronize();
}

void
dynamic_local::generate_aggs(void)
{
   for(node_set::iterator it(nodes->begin()); it != nodes->end(); ++it)
      generate_agg_node(*it);
}

dynamic_local::dynamic_local(const process_id id):
   static_local(id),
   nodes(NULL),
   nodes_mutex(NULL)
{
}
   
dynamic_local::~dynamic_local(void)
{
   assert(nodes != NULL);
   assert(nodes_mutex != NULL);
   
   delete nodes;
   delete nodes_mutex;
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
