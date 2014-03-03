
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
volatile bool base::stop_flag(false);

static void
cleanup_sched_key(void)
{
   pthread_key_delete(sched_key);
}

static bool
init(void)
{
   int ret(pthread_key_create(&sched_key, NULL));
   (void)ret;
   (void)started;
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

   init(All->NUM_THREADS);

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

base::base(const vm::process_id _id):
   id(_id),
	thread(NULL),
	state(this),
	iteration(0)
#ifdef INSTRUMENTATION
   , ins_state(statistics::NOW_ACTIVE)
#endif
{
}

}
