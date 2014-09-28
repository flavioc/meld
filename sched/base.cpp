
#include <algorithm>

#include "sched/base.hpp"
#include "process/work.hpp"
#include "db/tuple.hpp"
#include "vm/exec.hpp"
#include "process/machine.hpp"

using namespace std;
using namespace boost;
using namespace vm;
using namespace process;
using namespace db;

namespace sched
{
	
static __thread base *scheduler(NULL);
static __thread vm::state *state(NULL);

volatile bool base::stop_flag(false);

void
base::do_loop(void)
{
   db::node *node(NULL);

   while(true) {
      while((node = get_work())) {
         state->run_node(node);
         if(stop_flag) {
            killed_while_active();
            return;
         }
      }
      if(stop_flag) {
         killed_while_active();
         return;
      }

      assert_end_iteration();

      // cout << id << " -------- END ITERATION ---------" << endl;

      // false from end_iteration ends program
      if(!end_iteration())
         return;
   }
}
	
void
base::loop(void)
{
   // start process pool
   mem::ensure_pool();
   scheduler = this;
   state = mem::allocator<vm::state>().allocate(1);
   mem::allocator<vm::state>().construct(state, scheduler);

   init(All->NUM_THREADS);

   do_loop();

   assert_end();
   end();
   // cout << "DONE " << id << endl;
}

base*
base::get_scheduler(void)
{
   return scheduler;
}
	
void
base::start(void)
{
   if(id == 0) {
      thread = new boost::thread();
      loop();
   } else
      thread = new boost::thread(bind(&base::loop, this));
}

base::~base(void)
{
	delete thread;
}

base::base(const vm::process_id _id):
   id(_id),
	thread(NULL),
	iteration(0)
#ifdef INSTRUMENTATION
   , ins_state(statistics::NOW_ACTIVE)
#endif
{
}

}
