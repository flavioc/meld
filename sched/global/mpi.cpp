
#include "sched/global/mpi.hpp"

#ifdef COMPILE_MPI

#include <mpi.h>

#include "vm/state.hpp"
#include "process/router.hpp"
#include "utils/utils.hpp"

using namespace db;
using namespace process;
using namespace vm;
using namespace std;
using namespace utils;
using namespace boost;

namespace sched
{
   
static tokenizer *token;
static mutex tok_mutex;
   
void
mpi_static::assert_end_iteration(void)
{
   sstatic::assert_end_iteration();
   
   assert_mpi();
}

void
mpi_static::assert_end(void)
{
   sstatic::assert_end();
   
   assert_mpi();
}
   
void
mpi_static::begin_get_work(void)
{
   do_mpi_worker_cycle();
   do_mpi_leader_cycle();
}

void
mpi_static::messages_were_transmitted(const size_t total)
{
   mutex::scoped_lock lock(tok_mutex);
   
   assert(total > 0);
   
   token->messages_transmitted(total);
}

void
mpi_static::messages_were_received(const size_t total)
{
   mutex::scoped_lock lock(tok_mutex);
   assert(total > 0);
   token->messages_received(total);
}

void
mpi_static::new_mpi_message(node *node, simple_tuple *tpl)
{
   new_work(NULL, node, tpl);
}

bool
mpi_static::busy_wait(void)
{
   bool turned_inactive(false);
   
   transmit_messages();
   fetch_work();
   
   while(!has_work()) {

      update_pending_messages(true);

      if(!turned_inactive)
         turned_inactive = true;
      
      if(!token->busy_loop_token(turned_inactive))
         return false;
      
      fetch_work();
   }
   
   assert(has_work());
   
   return true;
}

void
mpi_static::work_found(void)
{
   assert(has_work());
}

bool
mpi_static::terminate_iteration(void)
{
   update_pending_messages(false);
   token->token_terminate_iteration();
   
   generate_aggs();
   
   return state::ROUTER->reduce_continue(has_work());
}
   
void
mpi_static::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(false);
}

void
mpi_static::new_work_remote(remote *rem, const node::node_id, message *msg)
{
   // this is buffered as the 0 thread id, since only one thread per proc exists
   buffer_message(rem, 0, msg);
}

mpi_static*
mpi_static::find_scheduler(const node::node_id)
{
   return this;
}

mpi_static::mpi_static(void):
   sstatic(0)
{
   token = new tokenizer();
}

mpi_static::~mpi_static(void)
{
}

mpi_static*
mpi_static::start(void)
{
   return new mpi_static();
}

#endif

}
