
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
state::setup(vm::tuple *tpl, db::node *n, const ref_count count)
{
   this->tuple = tpl;
   this->node = n;
   this->count = count;
}

}