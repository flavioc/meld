
#include <iostream>

#include "sched/mpi/message_buffer.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"
#include "conf.hpp"

using namespace vm;
using namespace std;
using namespace process;

namespace sched
{
   
#ifdef COMPILE_MPI
void
message_buffer::transmit_list(remote *rem, const process_id proc, message_set& ms, vm::all *all)
{
   assert(rem != NULL);
   assert(!ms.empty());
   assert(total > 0);
   
   ms.size();
   
   --total;
   
   assert(ms.get_storage_size() < MPI_BUFFER_THRESHOLD);
   req_obj obj(all->ROUTER->send(rem, proc, ms));
   
   req_handler.add_request(rem, obj);
   
#ifdef DEBUG_REMOTE
   cout << "Sent " << ms.size() << " messages to " << rem->get_rank() << ":" << proc << endl;
#endif
   
   assert(!req_handler.empty());
   
   ms.wipeout();
}

bool
message_buffer::insert(remote *rem, const process_id proc, message* msg, vm::all *all)
{
   map_messages::iterator it(map_rem.find(rem));
   
   assert(rem != NULL);
   assert(msg != NULL);
   
   if(it == map_rem.end()) {
      map_rem[rem] = map_procs();
      it = map_rem.find(rem);
   }
   
   map_procs& map_proc(it->second);
   
   map_procs::iterator it2(map_proc.find(proc));
   
   if(it2 == map_proc.end()) {
      map_proc[proc] = message_set();
      it2 = map_proc.find(proc);
   }
   
   message_set& ms(it2->second);

   bool transmitted(false);
   
   if(ms.get_storage_size() + msg->get_storage_size() >= MPI_BUFFER_THRESHOLD) {
      transmit_list(rem, proc, ms, all);
      transmitted = true;
   }
   
   if(ms.empty())
      ++total;
   ms.add(msg);
   
   return transmitted;
}

size_t
message_buffer::transmit(vm::all *all)
{
   if(total > 0) {
      const size_t ret(total);
      
      for(map_messages::iterator it(map_rem.begin()); it != map_rem.end(); ++it) {
         map_procs& map_proc(it->second);
         remote *rem(it->first);
         
         for(map_procs::iterator it2(map_proc.begin()); it2 != map_proc.end(); ++it2) {
            const process_id proc(it2->first);
            message_set& ms(it2->second);
         
            if(!ms.empty())
               transmit_list(rem, proc, ms, all);
         }
      }
      
      return ret;
   }
   
   return 0;
}
#endif

}
