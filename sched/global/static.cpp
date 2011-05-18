
#include "sched/global/static.hpp"
#include "vm/predicate.hpp"
#include "vm/program.hpp"
#include "process/remote.hpp"
#include "vm/state.hpp"

using namespace std;
using namespace vm;
using namespace db;
using namespace utils;
using namespace process;

namespace sched
{
   
void
sstatic::new_work(node *from, node *to, const simple_tuple *tpl, const bool is_agg)
{
   work_unit work = {to, tpl, is_agg};

   queue_work.push(work, tpl->get_strat_level());
}

void
sstatic::assert_end_iteration(void) const
{
   assert(!has_work());
}

void
sstatic::assert_end(void) const
{
   assert(!has_work());
}

bool
sstatic::get_work(work_unit& work)
{  
   if(!has_work()) {
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
      node_iteration(it->second);
}

void
sstatic::init(const size_t)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));
   
   for(; it != end; ++it)
      init_node(it->second);
}

void
sstatic::end(void)
{
}

sstatic::sstatic(const process_id _id):
   base(_id),
   queue_work(vm::predicate::MAX_STRAT_LEVEL)
{
}

sstatic::~sstatic(void)
{
}

}
