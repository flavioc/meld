
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
using namespace utils;

namespace process
{
   
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
#if 0
   if(remote::i_am_last_one())
      printf("Nodes per machine %ld\n", nodes_per_remote);
#endif
#endif
}

#ifdef COMPILE_MPI

pair<mpi::request, byte*>
router::send(remote *rem, const process_id& proc, const message_set& ms)
{
#ifdef DEBUG_SERIALIZATION_TIME
   utils::execution_time::scope s(serial_time);
#endif

   const int tag(get_thread_tag(proc));

#ifdef USE_MANUAL_SERIALIZATION
   const size_t msg_size(ms.storage_size());
#define MAX_BUF_SIZE 512
   byte *buf(new byte[msg_size]);
   
   assert(msg_size < MAX_BUF_SIZE);
   
   ms.pack(buf, msg_size, *world);
   
#ifdef DEBUG_REMOTE
   cout << "Serializing " << msg_size << " bytes of " << ms.size() << " messages" << endl;
#endif

   mpi::request ret;
   
   MPI_Isend(buf, msg_size, MPI_PACKED, rem->get_rank(), tag, *world, &ret.m_requests[0]);

   return pair_req(ret, buf);
#else
   return world->isend(rem->get_rank(), tag, ms);
#endif
}

message_set*
router::recv_attempt(const process_id proc)
{
#ifdef DEBUG_SERIALIZATION_TIME
   utils::execution_time::scope s(serial_time);
#endif

   const int tag(get_thread_tag(proc));

   optional<mpi::status> stat(world->iprobe(mpi::any_source, tag));
   
   if(stat) {
#ifdef USE_MANUAL_SERIALIZATION
      byte buf[MAX_BUF_SIZE];
      
      // this is a hack on Boost.MPI
      mpi::status stat;
      MPI_Recv(buf, MAX_BUF_SIZE, MPI_PACKED, mpi::any_source, tag, *world, &stat.m_status);
      
      message_set *ms(message_set::unpack(buf, MAX_BUF_SIZE, *world));
#else
      message_set *ms(new message_set());
      
      mpi::status stat(world->recv(mpi::any_source, tag, *ms));
      
#endif
      return ms;
   } else
      return NULL;
}

void
router::check_requests(vector_reqs& reqs)
{
   for(vector_reqs::iterator it(reqs.begin()); it != reqs.end(); ++it) {
      pair_req& r(*it);
      
      if(r.first.test()) {
         delete []r.second;
         it = reqs.erase(it);
      }
   }
}

void
router::fetch_updates(void)
{
   optional<mpi::status> st;
   
   while((st = world->iprobe(mpi::any_source, STATUS_TAG))) {
      remote_state state;
      mpi::status stat;
      
      stat = world->recv(mpi::any_source, STATUS_TAG, state);
      remote_states[stat.source()] = state;
   }
}

void
router::update_status(const remote_state state)
{
   // only send if different
   if(state != remote_states[remote::self->get_rank()]) {
      for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i)
         if(i != remote::self->get_rank())
            state_reqs.push_back(world->isend(remote_list[i]->get_rank(), STATUS_TAG, state));
      remote_states[remote::self->get_rank()] = state;
   }
}

void
router::send_status(const remote_state state)
{
   for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i)
      if(i != remote::self->get_rank())
         world->send(remote_list[i]->get_rank(), STATUS_TAG, state);
   remote_states[remote::self->get_rank()] = state;
}

void
router::fetch_new_states(void)
{
   for(remote::remote_id i(0); i != (remote::remote_id)world_size; ++i) {
      if(i != remote::self->get_rank()) {
         remote_state state;
         mpi::status stat;
         stat = world->recv(remote_list[i]->get_rank(), STATUS_TAG, state);
         remote_states[stat.source()] = state;
      }
   }
}

void
router::update_sent_states(void)
{
   list_state_reqs::iterator mark(mpi::test_some(state_reqs.begin(), state_reqs.end()));
   
   state_reqs.erase(mark, state_reqs.end());
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
router::base_constructor(const size_t num_threads, int argc, char **argv, const bool use_mpi)
{
#ifdef COMPILE_MPI
   if(argv != NULL && argc > 0 && use_mpi) {
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
   base_constructor(1, 0, NULL, false);
}

router::router(const size_t num_threads, int argc, char **argv, const bool use_mpi)
{
   base_constructor(num_threads, argc, argv, use_mpi);
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
#ifdef DEBUG_SERIALIZATION_TIME
      cout << "Serialization time: " << serial_time << endl;
#endif
   }
#endif
}

}
