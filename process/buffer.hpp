
#ifndef PROCESS_BUFFER_HPP
#define PROCESS_BUFFER_HPP

#include <list>

#include "process/message.hpp"
#include "vm/defs.hpp"
#include "mem/base.hpp"

namespace process
{
   
class remote;

class buffer: public mem::base<buffer>
{
private:
   
   typedef std::map<vm::process_id, message_set, std::less<vm::process_id>,
      mem::allocator< std::pair<vm::process_id, message_set> > > map_procs;
   typedef std::map<remote*, map_procs, std::less<remote*>,
      mem::allocator< std::pair<remote*, map_procs> > > map_messages;
   
   map_messages map_rem;
   size_t total;
   
   size_t transmit_list(remote *, const vm::process_id, message_set&);
   
public:
   
   inline const bool empty(void) const { return total == 0; }
   
   size_t insert(const vm::process_id, remote *, const vm::process_id, message *);
   
   size_t transmit(void);
   
   explicit buffer(void): total(0) {}
   
   ~buffer(void) {}
};

}

#endif
