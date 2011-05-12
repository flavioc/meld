
#include <iostream>

#include "sched/buffer.hpp"
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
buffer::transmit_list(remote *rem, const process_id proc, message_set& ms)
{
   ms.size();
   
   --total;
   
   reqs.push_back(state::ROUTER->send(rem, proc, ms));
   
#ifdef DEBUG_REMOTE
   cout << "Sent " << ms.size() << " messages to " << rem->get_rank() << ":" << proc << endl;
#endif
   
   ms.wipeout();
}

bool
buffer::insert(remote *rem, const process_id proc, message* msg)
{
   map_messages::iterator it(map_rem.find(rem));
   
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
   
   if(ms.empty())
      ++total;
   
   ms.add(msg);
   
   if(ms.size() >= MPI_BUFFER_THRESHOLD) {
      transmit_list(rem, proc, ms);
      return true;
   } else
      return false;
}

size_t
buffer::transmit(void)
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
               transmit_list(rem, proc, ms);
         }
      }
      
      return ret;
   }
   
   return 0;
}

void
buffer::update_received(void)
{
   if(!reqs.empty()) {
#ifdef DEBUG_REMOTE
      cout << "CHECKING " << reqs.size() << " requests\n";
#endif
      state::ROUTER->check_requests(reqs);
#ifdef DEBUG_REMOTE
      cout << "NOW I HAVE " << reqs.size() << " requests left\n";
#endif
   }
}
#endif

}
