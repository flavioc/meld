
#ifndef PROCESS_COUNTER_HPP
#define PROCESS_COUNTER_HPP

#include <map>

#include "mem/base.hpp"
#include "conf.hpp"

namespace process
{
   
class remote;
   
class counter: public mem::base<counter>
{
private:
   
   typedef std::map<vm::process_id, size_t, std::less<vm::process_id>,
                  mem::allocator<std::pair<vm::process_id, size_t> > > map_remote_proc;
   typedef std::map<remote*, map_remote_proc, std::less<remote*>,
                  mem::allocator<std::pair<remote*, map_remote_proc> > > map_remotes;
   map_remotes recv_remotes;
   
public:
   
   void flush_nonzero(void);
   
   void increment(remote *, const vm::process_id, const size_t);
   
   const bool everything_zero(void) const;
   
   explicit counter(void) {}
   
   ~counter(void) {}
};

}

#endif
