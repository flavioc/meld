
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
      
#ifdef COMPILE_MPI
   if(remote::i_am_last_one())
      printf("Nodes per machine %ld\n", nodes_per_remote);
#endif
}

#ifdef COMPILE_MPI
mpi::request
router::send(remote *rem, const process_id& proc, const message& msg)
{
   assert(0);
   return world->isend(rem->get_rank(), get_thread_tag(proc), msg);
}

mpi::request
router::send(remote *rem, const process_id& proc, const message_set& ms)
{
   return world->isend(rem->get_rank(), get_thread_tag(proc), ms);
}

message_set*
router::recv_attempt(const process_id proc, remote*& rem)
{
   mutex::scoped_lock l(mt);
   
   optional<mpi::status> stat(world->iprobe(mpi::any_source, get_thread_tag(proc)));
   
   if(stat) {
      message_set *ms(new message_set());
      
      mpi::status stat(world->recv(mpi::any_source, get_thread_tag(proc), *ms));
      
      rem = remote_list[stat.source()];
      
      return ms;
   } else
      return NULL;
}

void
router::check_requests(std::list<mpi::request>& reqs)
{
   reqs.erase(mpi::test_some(reqs.begin(), reqs.end()), reqs.end());
}

#endif
   
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
   }
#endif
}

void
router::update_status(const remote_state state)
{
#ifdef COMPILE_MPI
   if(state != remote_states[remote::self->get_rank()]) {
      for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i)
         if(i != remote::self->get_rank())
            world->send(remote_list[i]->get_rank(), STATUS_TAG, state);
         remote_states[remote::self->get_rank()] = state;
   }
#endif
}

void
router::synchronize(void)
{
#ifdef COMPILE_MPI
   world->barrier();
#endif
}

void
router::base_constructor(const size_t num_threads, int argc, char **argv)
{
#ifdef COMPILE_MPI
   if(argv != NULL && argc > 0) {
      assert(num_threads == 1); // limitation for now
      
#ifdef MPI_THREAD
      const int mpi_thread_support = MPI::Init_thread(argc, argv, MPI_THREAD_MULTIPLE);
   
      if(mpi_thread_support != MPI_THREAD_MULTIPLE)
         throw remote_error("No multithread support for MPI");
#endif

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
   if(world && env) {
      delete world;
      delete env;
   
#ifdef MPI_THREAD
      MPI::Finalize(); // must call this since MPI_Init_thread is not supported by boost
#endif
   }
#endif
}

}
