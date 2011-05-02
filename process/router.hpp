
#ifndef PROCESS_ROUTER_HPP
#define PROCESS_ROUTER_HPP

#include "conf.hpp"

#include <vector>
#include <list>
#include <stdexcept>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif

#include "process/remote.hpp"
#include "process/process.hpp"
#include "db/tuple.hpp"
#include "db/node.hpp"
#include "vm/defs.hpp"
#include "process/message.hpp"
#include "conf.hpp"

namespace process
{

class router
{
public:
   
   typedef bool remote_state;
   static const bool REMOTE_IDLE = true;
   static const bool REMOTE_ACTIVE = false;
   
private:
   
   typedef int remote_tag;
   
   static const remote_tag STATUS_TAG = 0;
   static const remote_tag THREAD_TAG = 100;
   static const remote_tag THREAD_DELAY_TAG = 500;
   
   static inline const remote_tag get_thread_tag(const vm::process_id id)
   {
      return THREAD_TAG + id;
   }
   
   static inline const remote_tag get_thread_delay_tag(const vm::process_id id)
   {
      return THREAD_DELAY_TAG + id;
   }
   
   std::vector<remote*> remote_list;   
   std::vector<bool> remote_states;
   
   size_t world_size;
   size_t nodes_per_remote;

#ifdef COMPILE_MPI
   boost::mpi::environment *env;
   boost::mpi::communicator *world;
#endif

   void base_constructor(const size_t, int, char **);
   
public:
   
   inline const bool use_mpi(void) const { return world_size > 1; }
   
   const bool finished(void) const;
   
   void fetch_updates(void);
   
   void update_status(const remote_state);
   
   void synchronize(void);
   
   void set_nodes_total(const size_t);
   
#ifdef COMPILE_MPI
   boost::mpi::request send(remote *, const vm::process_id&, const message&);
   boost::mpi::request send(remote *, const vm::process_id&, const message_set&);
   
   void check_requests(std::list<boost::mpi::request>&);
   
   message_set* recv_attempt(const vm::process_id, remote*&);
#endif
   
   remote* find_remote(const db::node::node_id) const;
   
   explicit router(const size_t, int, char**);
   explicit router(void);
   
   ~router(void);
};

}

#endif
