
#include "sched/dynamic_local.hpp"
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
   
static vector<dynamic_local*> others;
   
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
   while(!steal.empty())
      steal.pop();
}

dynamic_local*
dynamic_local::select_steal_target(void) const
{
   size_t idx(random_unsigned(others.size()));
   
   while(others[idx] == this)
      idx = random_unsigned(others.size());
   
   return others[idx];
}

bool
dynamic_local::busy_wait(void)
{
   static const size_t MAX_ASK_STEAL(3);
   
   bool turned_inactive(false);
   size_t asked_many(0);
   
   while(queue_nodes.empty()) {
      
      if(state::NUM_THREADS > 1 && asked_many < MAX_ASK_STEAL) {
         dynamic_local *target(select_steal_target());
         
         if(target->process_state == PROCESS_ACTIVE) {
            target->steal.push(this);
            ++asked_many;
         }
      }
      
      if(!turned_inactive && queue_nodes.empty()) {
         mutex::scoped_lock l(mutex);
         if(queue_nodes.empty()) {
            if(process_state == PROCESS_ACTIVE) {
               make_inactive();
               turned_inactive = true;
               if(term_barrier->all_finished())
                  return false;
            } else if(process_state == PROCESS_INACTIVE) {
               turned_inactive = true;
            }
         }
      }
      
      if(term_barrier->all_finished()) {
         assert(process_state == PROCESS_INACTIVE);
         return false;
      }
   }
   
   if(process_state == PROCESS_INACTIVE) {
      mutex::scoped_lock l(mutex);
      if(process_state == PROCESS_INACTIVE)
         make_active();
   }
   
   assert(process_state == PROCESS_ACTIVE);
   assert(!queue_nodes.empty());
   
   return true;
}

void
dynamic_local::change_node(thread_node *node, dynamic_local *asker)
{
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
   
   static const size_t MAX_PER_TIME(10);
   
   while(!steal.empty() && !queue_nodes.empty()) {
      assert(!steal.empty());
      assert(!queue_nodes.empty());
      
      dynamic_local *asker((dynamic_local*)steal.pop());
      
      assert(asker != NULL);
      
      //cout << "Answering request of " << (int)asker->get_id() << endl;
      size_t total_sent(0);
      
      while(!queue_nodes.empty() && total_sent < MAX_PER_TIME) {
         thread_node *node(queue_nodes.pop());

         assert(node != NULL);
         
         change_node(node, asker);
         
         ++total_sent;
      }
      
      assert(total_sent > 0);
      
      if(asker->process_state == PROCESS_INACTIVE) {
         mutex::scoped_lock lock(asker->mutex);
         if(asker->process_state == PROCESS_INACTIVE)
            asker->make_active();
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
   for(node_set::iterator it(nodes->begin()); it != nodes->end(); ++it) {
      node *no(*it);
      simple_tuple_list ls(no->generate_aggs());

      for(simple_tuple_list::iterator it2(ls.begin());
         it2 != ls.end();
         ++it2)
      {
         new_work(NULL, no, *it2, true);
      }
   }
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

vector<dynamic_local*>&
dynamic_local::start(const size_t num_threads)
{
   static_local::init_barriers(num_threads);
   others.resize(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      others[i] = new dynamic_local(i);
      
   return others;
}
   
}