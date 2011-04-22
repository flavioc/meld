
#include <iostream>
#include <boost/thread/mutex.hpp>

#include "process/router.hpp"
#include "vm/state.hpp"
#include "db/database.hpp"

using namespace process;
using namespace boost;
using namespace vm;
using namespace std;
using namespace db;

namespace process
{
   
static mutex mt;
   
void
router::set_nodes_total(const size_t total)
{
   nodes_per_remote = total / world_size;
   
   if(nodes_per_remote == 0)
      throw database_error("Number of nodes is less than the number of remote machines");
      
   // cache values for each remote
   
   for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i)
      remote_list[i]->cache_values(world_size, nodes_per_remote);
      
#if 0
   if(remote::i_am_last_one())
      printf("Nodes per machine %ld\n", nodes_per_remote);
#endif
}

void
router::send(remote* rem, const process::process_id proc, const message& msg)
{
   cout << "Sending to rem " << rem->get_rank() << " proc: " << proc << " TAG: " << get_thread_tag(proc) << ": ";
   cout << msg << endl;
#ifdef COMPILE_MPI
   world->send(rem->get_rank(), get_thread_tag(proc), msg);
#endif
}

message*
router::recv_attempt(const process::process_id proc)
{
#ifdef COMPILE_MPI
   mutex::scoped_lock l(mt);
   
   optional<mpi::status> stat(world->iprobe(mpi::any_source, get_thread_tag(proc)));
   
   if(stat) {
      message *msg(new message());
      
      world->recv(mpi::any_source, get_thread_tag(proc), *msg);
      cout << "NEW MESSAGE " << *msg << endl;
      return msg;
   } else
#endif
      return NULL;
}
   
remote*
router::find_remote(const node::node_id id) const
{
#ifdef COMPILE_MPI
   if(!use_mpi())
      return remote::self;
      
   const remote::remote_id rem(min(id / nodes_per_remote, world_size-1));

   return remote_list[rem];
#else
   return remote::self;
#endif
}

const bool
router::finished(void) const
{
   for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i)
      if(remote_states[i] == REMOTE_ACTIVE)
         return false;
   
   return true;
}

void
router::fetch_updates(void)
{
#ifdef COMPILE_MPI
   optional<mpi::status> st;
   
   mutex::scoped_lock l(mt);
   
   while((st = world->iprobe(mpi::any_source, STATUS_TAG))) {
      remote_state state;
      mpi::status stat;
      
      stat = world->recv(mpi::any_source, STATUS_TAG, state);
      remote_states[stat.source()] = state;
      
      cout << "RECEIVED STATE FROM " << stat.source() << ": " << state << endl;
   }
#endif
}

void
router::update_status(const remote_state state)
{
#ifdef COMPILE_MPI
   for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i)
      if(i != remote::self->get_rank())
         world->send(remote_list[i]->get_rank(), STATUS_TAG, state);
   remote_states[remote::self->get_rank()] = state;
#endif
}

void
router::finish(void)
{
#ifdef COMPILE_MPI
   world->barrier();
#endif
}

void
router::base_constructor(const size_t num_threads, int argc, char **argv)
{
#ifdef COMPILE_MPI
   if(USE_MPI && argv != NULL && argc > 0) {
      const int mpi_thread_support = MPI::Init_thread(argc, argv, MPI_THREAD_MULTIPLE);
   
      if(mpi_thread_support != MPI_THREAD_MULTIPLE)
         throw remote_error("No multithread support for MPI");
      
      env = new mpi::environment(argc, argv);
      world = new mpi::communicator();
   
      world_size = world->size();
   
      if(world->rank() == 0)
         cout << "WORLD SIZE: " << world_size << endl;
   
      remote_list.resize(world_size);
      for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i) {
         size_t nthreads_other;
         if(i == world->rank())
            nthreads_other = num_threads;
         mpi::broadcast(*world, nthreads_other, i);
         remote_list[i] = new remote(i, nthreads_other);
      }
   
      state::REMOTE = remote::self = remote_list[world->rank()];
   } else
#endif
   {
      world_size = 1;
      remote_list.resize(world_size);
      remote_list[0] = new remote(0, num_threads);
#ifdef COMPILE_MPI
      env = NULL;
      world = NULL;
#endif
      state::REMOTE = remote::self = remote_list[0];
   }
   
   state::ROUTER = this;
   
   remote_states.resize(world_size);
   
   for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i)
      remote_states[i] = REMOTE_ACTIVE;
}

router::router(void)
{
   base_constructor(1, 0, NULL);
}

router::router(const size_t num_threads, int argc, char **argv)
{
   base_constructor(num_threads, argc, argv);
}

router::~router(void)
{
   for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i)
      delete remote_list[i];

#ifdef COMPILE_MPI
   if(USE_MPI && world && env) {
      delete world;
      delete env;
   
      MPI::Finalize(); // must call this since MPI_Init_thread is not supported by boost
   }
#endif
}

}