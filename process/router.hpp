
#ifndef PROCESS_ROUTER_HPP
#define PROCESS_ROUTER_HPP

#include <vector>
#include <boost/mpi.hpp>
#include <stdexcept>

#include "process/remote.hpp"
#include "process/process.hpp"
#include "db/tuple.hpp"
#include "db/node.hpp"
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
   
   static inline const remote_tag get_thread_tag(const process::process_id id)
   {
      return THREAD_TAG + id;
   }
   
   std::vector<remote*> remote_list;   
   std::vector<bool> remote_states;
   
   size_t world_size;
   size_t nodes_per_remote;
   boost::mpi::environment *env;
   boost::mpi::communicator *world;
   
public:
   
   inline const bool use_mpi(void) const { return USE_MPI && world_size > 0; }
   
   const bool finished(void) const;
   
   void fetch_updates(void);
   
   void update_status(const remote_state);
   
   void finish(void);
   
   void set_nodes_total(const size_t);
   
   void send(remote*, const process::process_id, const message&);
   
   message* recv_attempt(const process::process_id);
   
   remote* find_remote(const db::node::node_id) const;
   
   explicit router(const size_t, int, char**);
   
   ~router(void);
};

}

#endif
