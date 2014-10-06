
#ifndef PROCESS_ROUTER_HPP
#define PROCESS_ROUTER_HPP

#include "conf.hpp"

#include <vector>
#include <list>
#include <stdexcept>

#include "process/remote.hpp"
#include "db/tuple.hpp"
#include "db/node.hpp"
#include "vm/defs.hpp"
#include "utils/time.hpp"
#include "utils/types.hpp"
#include "process/remote.hpp"

namespace process
{

class router
{
public:
   
   typedef bool remote_state;
   static const bool REMOTE_IDLE = true;
   static const bool REMOTE_ACTIVE = false;
   
private:
   
   std::vector<remote*> remote_list;
   
   size_t world_size;
   size_t nodes_per_remote;

   void base_constructor(const size_t, int, char **, const bool);
   
public:
   
   inline bool use_mpi(void) const { return world_size > 1; }
   
   void set_nodes_total(const size_t);

   remote* find_remote(const db::node::node_id) const;
   
   explicit router(const size_t, int, char**, const bool);
   explicit router(void);
   
   ~router(void);
};

}

#endif
