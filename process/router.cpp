
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
using namespace sched;

namespace process
{
   
static mutex mpi_mutex;
   
   
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

   assert(rem != NULL);

   const int tag(get_thread_tag(proc));

   const size_t msg_size(ms.get_storage_size());
   byte *buf(new byte[msg_size]);
   
   assert(msg_size < MPI_BUF_SIZE);
   
   ms.pack(buf, msg_size, *world);
   
#ifdef DEBUG_REMOTE
   cout << "Serializing " << msg_size << " bytes of " << ms.size() << " messages" << endl;
#endif

   mpi::request ret;
   mutex::scoped_lock lock(mpi_mutex);
   
   MPI_Isend(buf, msg_size, MPI_PACKED, rem->get_rank(), tag, *world, &ret.m_requests[0]);

   return pair_req(ret, buf);
}

message_set*
router::recv_attempt(const process_id proc)
{
#ifdef DEBUG_SERIALIZATION_TIME
   utils::execution_time::scope s(serial_time);
#endif

   const int tag(get_thread_tag(proc));

   optional<mpi::status> stat;
   
   {
      mutex::scoped_lock lock(mpi_mutex);
      
      stat = world->iprobe(mpi::any_source, tag);
   }
   
   if(stat) {
      // this is a hack on Boost.MPI
      mpi::status stat;
      {
         mutex::scoped_lock lock(mpi_mutex);
         MPI_Recv(recv_buf, MPI_BUF_SIZE, MPI_PACKED, mpi::any_source, tag, *world, &stat.m_status);
      }
      
      message_set *ms(message_set::unpack(recv_buf, MPI_BUF_SIZE, *world));
      return ms;
   } else
      return NULL;
}

void
router::check_requests(vector_reqs& reqs, const bool test)
{
   assert(!reqs.empty());
   
   mutex::scoped_lock lock(mpi_mutex);
   
   for(vector_reqs::iterator it(reqs.begin()); it != reqs.end(); ++it) {
      pair_req& r(*it);
      
      if((test && r.first.test()) || !test) {
         delete []r.second;
         it = reqs.erase(it);
      }
   }
}

void
router::send_token(const token& tok)
{
   mutex::scoped_lock lock(mpi_mutex);
   
   const remote::remote_id left(remote::self->left_remote_id());
   
   world->send(left, TOKEN_TAG, tok);
}

bool
router::receive_token(token& global_tok)
{
   const remote::remote_id right(remote::self->right_remote_id());
   
   optional<mpi::status> st;
   mutex::scoped_lock lock(mpi_mutex);
   
   while((st = world->iprobe(right, TOKEN_TAG))) {
      world->recv(right, TOKEN_TAG, global_tok);
      
      return true;
   }
      
   return false;
}

void
router::send_end_iteration(const size_t stage, const remote::remote_id target)
{
   world->send(target, TERMINATE_ITERATION_TAG, stage);
}

bool
router::received_end_iteration(size_t& stage, const remote::remote_id source)
{
   assert(!remote::self->is_leader());
   
   optional<mpi::status> st;
   
   while((st = world->iprobe(source, TERMINATE_ITERATION_TAG))) {
      world->recv(source, TERMINATE_ITERATION_TAG, stage);
      
      return true;
   }
   
   return false;
}

void
router::receive_end_iteration(const remote::remote_id source)
{
   size_t stage; // UNUSED
   world->recv(source, TERMINATE_ITERATION_TAG, stage);
}

bool
router::reduce_continue(const bool more_work)
{
   return mpi::all_reduce(*world, more_work, logical_or<bool>());
}

void
router::wait_print_order(void)
{
   if(remote::self->is_leader())
      return;
      
   world->recv(remote::self->left_remote_id(), PRINT_TAG);
}

void
router::send_print_order(void)
{
   if(remote::self->is_last())
      return;
      
   world->send(remote::self->right_remote_id(), PRINT_TAG);
}
#endif
   
remote*
router::find_remote(const node::node_id id) const
{
#ifdef COMPILE_MPI
   if(!use_mpi())
      return remote::self;
      
   assert(id <= state::DATABASE->max_id());
   
   assert(nodes_per_remote > 0);
   assert(world_size > 1);
   
   const remote::remote_id rem(min(id / nodes_per_remote, world_size-1));

   //printf("Returning remote %d from id %d\n", rem, id);
   return remote_list[rem];
#else
   return remote::self;
#endif
}

void
router::base_constructor(const size_t num_threads, int argc, char **argv, const bool use_mpi)
{
#ifdef COMPILE_MPI
   if(argv != NULL && argc > 0 && use_mpi) {
      static const int mpi_required_support(MPI_THREAD_FUNNELED);
      int mpi_thread_support;
      
      MPI_Init_thread(&argc, &argv, mpi_required_support, &mpi_thread_support);
   
      if(mpi_thread_support != mpi_required_support)
         throw remote_error("No multithread support for MPI");

      env = new mpi::environment(argc, argv);
      world = new mpi::communicator();
   
      world_size = world->size();
   
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
   
   remote::world_size = world_size;
   
   state::ROUTER = this;
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
   
      MPI_Finalize(); // must call this since MPI_Init_thread is not supported by boost
      
#ifdef DEBUG_SERIALIZATION_TIME
      cout << "Serialization time: " << serial_time << endl;
#endif
   }
#endif
}

}
