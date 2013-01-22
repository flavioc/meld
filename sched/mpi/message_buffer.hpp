
#ifndef SCHED_MPI_MESSAGE_BUFFER_HPP
#define SCHED_MPI_MESSAGE_BUFFER_HPP

#include "conf.hpp"

#include <list>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif

#include "vm/defs.hpp"
#include "vm/all.hpp"
#include "mem/base.hpp"
#include "utils/types.hpp"
#include "sched/mpi/message.hpp"
#include "sched/mpi/request.hpp"

namespace process
{
   class remote;
}

namespace sched
{

#ifdef COMPILE_MPI

class message_buffer: public mem::base
{
private:
   
   typedef std::map<vm::process_id, message_set, std::less<vm::process_id>,
      mem::allocator< std::pair<vm::process_id, message_set> > > map_procs;
   typedef std::map<process::remote*, map_procs, std::less<process::remote*>,
      mem::allocator< std::pair<process::remote*, map_procs> > > map_messages;
   
   map_messages map_rem;
   request_handler req_handler;
   size_t total;
   
   void transmit_list(process::remote *, const vm::process_id, message_set&, vm::all *);
   
public:

   MEM_METHODS(message_buffer)
   
   inline bool empty(void) const { return total == 0; }
   
   inline bool all_received(void) const { return req_handler.empty(); }
   
   inline void update_received(const bool test, vm::all *all)
   {
      if(!req_handler.empty())
         req_handler.flush(test, all);
   }
   
   bool insert(process::remote *, const vm::process_id, message *, vm::all *);
   
   size_t transmit(vm::all *);
   
   explicit message_buffer(void): total(0) {}
   
   ~message_buffer(void) {}
};
#endif

}

#endif
