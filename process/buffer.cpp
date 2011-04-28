
#include <iostream>

#include "process/buffer.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"
#include "conf.hpp"

using namespace vm;
using namespace std;

namespace process
{
   
size_t
buffer::transmit_list(remote *rem, const process_id proc, message_set& ms)
{
   const size_t ret(ms.size());
   
   total -= ret;
   
   state::ROUTER->send(rem, proc, ms);
   
#ifdef DEBUG_REMOTE
   cout << "Sent " << ms.size() << " messages to " << rem->get_rank() << ":" << proc << endl;
#endif
   
   ms.wipeout();
   return ret;
}

size_t
buffer::insert(const process_id source, remote *rem, const process_id proc, message* msg)
{
   map_messages::iterator it(map_rem.find(rem));
   
   if(it == map_rem.end()) {
      map_rem[rem] = map_procs();
      it = map_rem.find(rem);
   }
   
   map_procs& map_proc(it->second);
   
   map_procs::iterator it2(map_proc.find(proc));
   
   if(it2 == map_proc.end()) {
      map_proc[proc] = message_set(source);
      it2 = map_proc.find(proc);
   }
   
   message_set& ms(it2->second);
   
   ++total;
   
   ms.add(msg);
   
   static const size_t BUFFER_THRESHOLD(25);
   
   if(ms.size() >= BUFFER_THRESHOLD)
      return transmit_list(rem, proc, ms);
   else
      return 0;
}

size_t
buffer::transmit(void)
{
   size_t ret(total);
   
   if(total > 0) {
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
   }
   
   return ret;
}

}
