
#include "sched/static.hpp"
#include "vm/predicate.hpp"
#include "vm/program.hpp"
#include "process/remote.hpp"
#include "vm/state.hpp"

using namespace vm;
using namespace db;
using namespace utils;
using namespace process;

namespace sched
{
   
void
sstatic::new_work(node *node, const simple_tuple *tpl, const bool is_agg)
{
   work_unit work = {node, tpl, is_agg};

   queue_work.push(work);
}

void
sstatic::new_work_agg(node *node, const simple_tuple *tpl)
{
   work_unit work = {node, tpl, true};

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
   if(queue_work.empty()) {
      if(!busy_wait())
         return false;

      work_found();
   }
   
   work = queue_work.pop();
   
   return true;
}

void
sstatic::generate_aggs(void)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));
   
   for(; it != end; ++it)
   {
      node *no(it->second);
      simple_tuple_list ls(no->generate_aggs());

      for(simple_tuple_list::iterator it2(ls.begin());
         it2 != ls.end();
         ++it2)
      {
         //cout << no->get_id() << " GENERATE " << **it2 << endl;
         new_work_agg(no, *it2);
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
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));
   
   for(; it != end; ++it)
   {
      node *cur_node(it->second);
      new_work(cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
   }
}

void
sstatic::end(void)
{
}

sstatic::sstatic(const process_id _id):
   base(_id),
   iteration(0)
{
}

sstatic::~sstatic(void)
{
}

}