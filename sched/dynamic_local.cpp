
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
dynamic_local::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(process_state == PROCESS_ACTIVE);
   assert(node != NULL);
   assert(stuple != NULL);
   assert(scheduler == NULL);
   
   thread_node *tnode((thread_node*)node);
   
   tnode->add_work(stuple, false);
   
   if(!tnode->in_queue()) {
      mutex::scoped_lock lock(tnode->mtx);
      if(!tnode->in_queue()) {
         dynamic_local *owner((dynamic_local*)tnode->get_owner());
         owner->add_to_queue(tnode);
         
         if(this != owner && 
            owner->process_state == PROCESS_INACTIVE)
         {
            mutex::scoped_lock lock2(owner->mutex);
            
            if(owner->process_state == PROCESS_INACTIVE)
               owner->make_active();
            assert(owner->process_state == PROCESS_ACTIVE);
         }
         
         assert(tnode->in_queue());
      }
   }
}

dynamic_local*
dynamic_local::select_steal_target(void) const
{
   size_t idx(random_unsigned(others.size()));
   
   while(others[idx] == this)
      idx = random_unsigned(others.size());
   
   return others[idx];
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