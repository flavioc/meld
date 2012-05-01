
#include "vm/state.hpp"

using namespace vm;
using namespace db;
using namespace process;
using namespace std;
using namespace runtime;

namespace vm
{

program *state::PROGRAM = NULL;
database *state::DATABASE = NULL;
machine *state::MACHINE = NULL;
remote *state::REMOTE = NULL;
router *state::ROUTER = NULL;
size_t state::NUM_THREADS = 0;
size_t state::NUM_PREDICATES = 0;
size_t state::NUM_NODES = 0;
size_t state::NUM_NODES_PER_PROCESS = 0;

bool
state::linear_tuple_can_be_used(vm::tuple *tpl, const vm::ref_count max) const
{
   for(list_linear::const_iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      const pair_linear& p(*it);
      
      if(p.first == tpl)
         return p.second < max;
   }
   
   return true; // not found, first time
}

void
state::using_new_linear_tuple(vm::tuple *tpl)
{
   for(list_linear::iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      pair_linear& p(*it);
      
      if(p.first == tpl) {
         p.second++;
         return;
      }
   }
   
   // new
   used_linear_tuples.push_front(pair_linear(tpl, 1));
}

void
state::no_longer_using_linear_tuple(vm::tuple *tpl)
{
   for(list_linear::iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      pair_linear &p(*it);
      
      if(p.first == tpl) {
         p.second--;
         if(p.second == 0)
            used_linear_tuples.erase(it);
         return;
      }
   }
   
   assert(false);
}

void
state::purge_lists(void)
{
#define PURGE_LIST(TYPE) \
   for(list<TYPE ## _list*>::iterator it(free_ ## TYPE ## _list.begin()); it != free_ ## TYPE ## _list.end(); ++it) { \
      TYPE ## _list *ls(*it); \
      assert(ls != NULL); \
      if(ls->zero_refs()) { ls->destroy(); } \
   } \
   free_ ## TYPE ## _list.clear()
   
   PURGE_LIST(float);
   PURGE_LIST(int);
   PURGE_LIST(node);
}

void
state::cleanup(void)
{
   purge_lists();
   assert(used_linear_tuples.empty());
}

void
state::setup(vm::tuple *tpl, db::node *n, const ref_count count)
{
   this->tuple = tpl;
   this->tuple_leaf = NULL;
   this->node = n;
   this->count = count;
   this->is_linear = tpl->is_linear();
   assert(used_linear_tuples.empty());
   this->used_linear_tuples.clear();
}

}
