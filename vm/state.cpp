
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
state::linear_tuple_is_being_used(vm::tuple *tpl) const
{
   for(list<vm::tuple*>::const_iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      if(*it == tpl)
         return true;
   }
   
   return false;
}

void
state::using_new_linear_tuple(vm::tuple *tpl)
{
   used_linear_tuples.push_front(tpl);
}

void
state::no_longer_using_linear_tuple(vm::tuple *tpl)
{
   for(list<vm::tuple*>::iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      if(*it == tpl) {
         used_linear_tuples.erase(it);
         return;
      }
      
      assert(false);
   }
   
   assert(false);
}

void
state::purge_lists(void)
{
#define PURGE_LIST(TYPE) \
   for(list<TYPE ## _list*>::iterator it(free_ ## TYPE ## _list.begin()); it != free_ ## TYPE ## _list.end(); ++it) { \
      TYPE ## _list *ls(*it); \
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