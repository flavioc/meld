
#include <algorithm>

#include "sched/base.hpp"
#include "process/work.hpp"
#include "db/tuple.hpp"
#include "db/neighbor_agg_configuration.hpp"
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
base::do_tuple_add(node *node, vm::tuple *tuple, const ref_count count)
{
   if(tuple->is_linear()) {
      state.setup(tuple, node, count);
      const byte_code code(state.all->PROGRAM->get_predicate_bytecode(tuple->get_predicate_id()));
      const execution_return ret(execute_bytecode(code, state));
      
      if(ret == EXECUTION_CONSUMED) {
         delete tuple;
      } else {
         node->add_tuple(tuple, count);
		}
   } else {
      const bool is_new(node->add_tuple(tuple, count));

      if(is_new) {
         // set vm state
         state.setup(tuple, node, count);
         byte_code code(state.all->PROGRAM->get_predicate_bytecode(tuple->get_predicate_id()));
         execute_bytecode(code, state);
      } else
         delete tuple;
   }
}

void
base::do_agg_tuple_add(node *node, vm::tuple *tuple, const ref_count count)
{
   const predicate *pred(tuple->get_predicate()); // get predicate here since tuple can be deleted!
   agg_configuration *conf(node->add_agg_tuple(tuple, count));
   const aggregate_safeness safeness(pred->get_agg_safeness());
   
   switch(safeness) {
      case AGG_UNSAFE: return;
      case AGG_IMMEDIATE: {
         simple_tuple_list list;
         
         conf->generate(pred->get_aggregate_type(), pred->get_aggregate_field(), list);

         for(simple_tuple_list::iterator it(list.begin()); it != list.end(); ++it) {
            simple_tuple *tpl(*it);
            new_work_agg(node, tpl);
         }
         return;
      }
      break;
#if 0
      case AGG_LOCALLY_GENERATED: {
         const strat_level level(pred->get_agg_strat_level());
         
         if(node->get_local_strat_level() < level) {
            return;
         }
      }
      break;
#endif
      case AGG_NEIGHBORHOOD:
      case AGG_NEIGHBORHOOD_AND_SELF: {
         const neighbor_agg_configuration *neighbor_conf(dynamic_cast<neighbor_agg_configuration*>(conf));
   
         if(!neighbor_conf->all_present()) {
            return;
         }
      }
      break;
      default: return;
   }

   simple_tuple_list list;
   conf->generate(pred->get_aggregate_type(), pred->get_aggregate_field(), list);

   for(simple_tuple_list::iterator it(list.begin()); it != list.end(); ++it) {
      simple_tuple *tpl(*it);
      
      assert(tpl->get_count() > 0);
      new_work_agg(node, tpl);
   }
}

void
base::do_work(db::node *node)
{
   state.run_node(node);

#if 0
   auto_ptr<const simple_tuple> stuple(w.get_tuple()); // this will delete tuple automatically
   vm::tuple *tuple = stuple->get_tuple();
   ref_count count = stuple->get_count();
   node *node(w.get_node());

   if(stuple->must_be_deleted()) {
		delete tuple;
	   return;
	}

	assert(count != 0);

#ifdef DEBUG_MODE
   cout << node->get_id() << " " << *tuple << endl;

	string name(tuple->get_predicate()->get_name());

   if(name == string("rv"))
      state.print_instrs = true;
#endif

   if(w.locally_generated())
      node->pop_auto(stuple.get());

   if(count > 0) {
      if(tuple->is_action()) {
         state::MACHINE->run_action(this, node, tuple);
      } else if(tuple->is_aggregate() && !w.force_aggregate())
         do_agg_tuple_add(node, tuple, count);
      else
         do_tuple_add(node, tuple, count);
   } else {
      assert(!tuple->is_action());

      count = -count;

      if(tuple->is_aggregate() && !w.force_aggregate()) {
         node->remove_agg_tuple(tuple, count);
      } else {
         node::delete_info deleter(node->delete_tuple(tuple, count));

         if(deleter.to_delete()) { // to be removed
            state.setup(tuple, node, -count);
            execute_bytecode(state.PROGRAM->get_predicate_bytecode(tuple->get_predicate_id()), state);
            deleter();
         } else
            delete tuple; // as in the positive case, nothing to do
      }
   }
#endif
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
