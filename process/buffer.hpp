
#ifndef PROCESS_BUFFER_HPP
#define PROCESS_BUFFER_HPP

#include <list>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif

#include "process/message.hpp"
#include "vm/defs.hpp"
#include "mem/base.hpp"

namespace process
{
   
class remote;

#ifdef COMPILE_MPI
typedef std::list<boost::mpi::request> vector_reqs;
#endif

class buffer: public mem::base<buffer>
{
private:
   
   typedef std::map<vm::process_id, message_set, std::less<vm::process_id>,
      mem::allocator< std::pair<vm::process_id, message_set> > > map_procs;
   typedef std::map<remote*, map_procs, std::less<remote*>,
      mem::allocator< std::pair<remote*, map_procs> > > map_messages;
   
   map_messages map_rem;
   vector_reqs reqs;
   size_t total;
   
   void transmit_list(remote *, const vm::process_id, message_set&);
   
public:
   
   inline const bool empty(void) const { return total == 0; }
   
   inline const bool all_received(void) const { return reqs.empty(); }
   
   void update_received(void);
   
   void insert(const vm::process_id, remote *, const vm::process_id, message *);
   
   void transmit(void);
   
   explicit buffer(void): total(0) {}
   
   ~buffer(void) {}
};

}

#endif
