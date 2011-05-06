
#ifndef SCHED_BUFFER_HPP
#define SCHED_BUFFER_HPP

#include <list>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif

#include "process/message.hpp"
#include "vm/defs.hpp"
#include "mem/base.hpp"
#include "utils/types.hpp"
#include "process/request.hpp"

namespace process
{
   class remote;
}

namespace sched
{

#ifdef COMPILE_MPI

class buffer: public mem::base<buffer>
{
private:
   
   typedef std::map<vm::process_id, process::message_set, std::less<vm::process_id>,
      mem::allocator< std::pair<vm::process_id, process::message_set> > > map_procs;
   typedef std::map<process::remote*, map_procs, std::less<process::remote*>,
      mem::allocator< std::pair<process::remote*, map_procs> > > map_messages;
   
   map_messages map_rem;
   process::vector_reqs reqs;
   size_t total;
   
   void transmit_list(process::remote *, const vm::process_id, process::message_set&);
   
public:
   
   inline const bool empty(void) const { return total == 0; }
   
   inline const bool all_received(void) const { return reqs.empty(); }
   
   void update_received(void);
   
   bool insert(process::remote *, const vm::process_id, process::message *);
   
   size_t transmit(void);
   
   explicit buffer(void): total(0) {}
   
   ~buffer(void) {}
};
#endif

}

#endif
