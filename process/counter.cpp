
#include "process/remote.hpp"
#include "process/counter.hpp"
#include "process/router.hpp"

using namespace std;
using namespace vm;

namespace process
{
   
void
counter::increment(remote *rem, const process_id source, const size_t total)
{
   map_remotes::iterator it(recv_remotes.find(rem));
   
   if(it == recv_remotes.end()) {
      recv_remotes[rem] = map_remote_proc();
      it = recv_remotes.find(rem);
   }
   
   map_remote_proc &proc_map(it->second);
   map_remote_proc::iterator it2(proc_map.find(source));
   
   if(it2 == proc_map.end()) {
      proc_map[source] = 0;
      it2 = proc_map.find(source); 
   }
   
   it2->second += total;
   
#ifdef DEBUG_REMOTE
   cout << "Remotes from " << it->first->get_rank() << ":" << source << " is now " << it2->second << endl;
#endif   

   assert(source == 0); // XXX: for now...
   assert(proc_map[source] > 0);
}

const bool
counter::everything_zero(void) const
{
   for(map_remotes::const_iterator it(recv_remotes.begin()); it != recv_remotes.end(); ++it) {
      for(map_remote_proc::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2) {
         if(it2->second > 0)
            return false;
      }
   }
   return true;
}

void
counter::flush_nonzero(void)
{
   for(map_remotes::iterator it(recv_remotes.begin()); it != recv_remotes.end(); ++it) {
      for(map_remote_proc::iterator it2(it->second.begin()); it2 != it->second.end(); ++it2) {
         if(it2->second > 0) {
#ifdef DEBUG_REMOTE
            cout << "Sending to " << it->first->get_rank() << " that " << it2->second << " messages sent by him were processed" << endl;
#endif
            state::ROUTER->send_processed_messages(it->first, it2->first, it2->second);
            it2->second = 0;
         }
      }
   }
}

}