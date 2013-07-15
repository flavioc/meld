
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
	
static bool init(void);

pthread_key_t sched_key;
static bool started(init());

static void
cleanup_sched_key(void)
{
   pthread_key_delete(sched_key);
}

static bool
init(void)
{
   int ret(pthread_key_create(&sched_key, NULL));
   assert(ret == 0);
   atexit(cleanup_sched_key);
   return true;
}
	
void
base::do_work(db::node *node)
{
   state.run_node(node);
}


void
base::do_loop(void)
{
   db::node *node(NULL);

   while(true) {
      while((node = get_work())) {
         do_work(node);
         finish_work(node);
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

   init(state.all->NUM_THREADS);

   do_loop();

   assert_end();
   end();
   // cout << "DONE " << id << endl;
}

base*
base::get_scheduler(void)
{
   sched::base *s((sched::base*)pthread_getspecific(sched_key));
   return s;
}
	
void
base::start(void)
{
   pthread_setspecific(sched_key, this);
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

base::base(const vm::process_id _id, vm::all *_all):
   id(_id),
	thread(NULL),
	state(this, _all),
	iteration(0)
#ifdef INSTRUMENTATION
   , processed_facts(0), sent_facts(0), ins_state(statistics::NOW_ACTIVE)
#endif
{

}

}
