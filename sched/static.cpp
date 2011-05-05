
#include "sched/static.hpp"
#include "vm/predicate.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"

using namespace vm;
using namespace db;
using namespace utils;

namespace sched
{
   
void
sstatic::new_work(node *node, const simple_tuple *tpl, const bool is_agg)
{
   work_unit work = {node, tpl, is_agg};

   queue_work.push(work);
}

void
sstatic::assert_end_iteration(void) const
{
   assert(queue_work.empty());
}

void
sstatic::assert_end(void) const
{
   assert(queue_work.empty());
}

bool
sstatic::get_work(work_unit& work)
{
   begin_get_work();
   
   if(queue_work.empty()) {
      if(!busy_wait())
         return false;

      work_found();
   }
   
   work = queue_work.pop();
   
   return true;
}

void
sstatic::add_node(node *node)
{
   nodes.push_back(node);
   
   if(nodes_interval == NULL)
      nodes_interval = new interval<node::node_id>(node->get_id(), node->get_id());
   else
      nodes_interval->update(node->get_id());
}

void
sstatic::generate_aggs(void)
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
         new_work(no, *it2, true);
      }
   }
}

bool
sstatic::terminate_iteration(void)
{
   ++iteration;
   return true;
}

void
sstatic::init(const size_t)
{  
   predicate *init_pred(state::PROGRAM->get_init_predicate());
   
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end();
      ++it)
   {
      node *cur_node(*it);
      
      new_work(cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
   }
}

void
sstatic::end(void)
{
}

sstatic::sstatic(void):
   nodes_interval(NULL),
   iteration(0)
{
}

sstatic::~sstatic(void)
{
   delete nodes_interval;
}

}