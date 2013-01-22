
#include "sched/mpi/request.hpp"
#include "process/remote.hpp"
#include "process/router.hpp"
#include "vm/state.hpp"

using namespace process;
using namespace vm;
using namespace utils;
using namespace mem;

#ifdef COMPILE_MPI

namespace sched
{
   
static const size_t REQUESTS_PER_ROUND(5);

void
request_handler::clear(void)
{
   remote::remote_id self(remote::self->get_rank());
   
   for(remote::remote_id i(0); i < (remote::remote_id)remote::world_size; ++i) {
      if(i != self) {
         for(list_reqs::iterator it(all_reqs[i].begin()); it != all_reqs[i].end(); ++it) {
            req_obj& obj(*it);
            allocator<byte>().deallocate(obj.mem, obj.mem_size);
         }
      }
   }
}

void
request_handler::flush(const bool test, vm::all *all)
{
   if(!test) {
      clear();
      return;
   }
   
   remote::remote_id self(remote::self->get_rank());
   
   for(remote::remote_id i(0); i < (remote::remote_id)remote::world_size; ++i) {
      if(i != self) {
         list_reqs& ls(all_reqs[i]);
         
         if(ls.empty())
            continue;
         
         list_reqs::iterator begin(ls.begin());
         list_reqs::iterator it(begin);
         list_reqs::iterator end(ls.end());
         
         size_t j(0);
         
         for(; it != end; ) {
            MPI_Request reqs[requests_per_round];
            byte *mems[requests_per_round];
            size_t mem_sizes[requests_per_round];
            list_reqs::iterator new_it(it);
            
            assert(requests_per_round > 0);
            size_t k(0);
            for(; new_it != end && k < requests_per_round; ++k, ++new_it) {
               req_obj& obj(*new_it);
               reqs[k] = obj.mpi_req;
               mems[k] = obj.mem;
               mem_sizes[k] = obj.mem_size;
            }
            
            if(k > 0 && all->ROUTER->was_received(k, reqs)) {
               for(size_t x(0); x < k; ++x)
                  allocator<byte>().deallocate(mems[x], mem_sizes[x]);
               assert(k <= requests_per_round && k > 0);
               j += k;
               assert(j <= ls.size());
               assert(it != new_it);
               it = new_it;
               ++requests_per_round;
            } else {
               // failed here, not worth continuing check
               if(requests_per_round > 1)
                  --requests_per_round;
               break;
            }
         }
         
         // we can arrive here if test == false or test == true (and all were received)
         total -= j;
         ls.erase(begin, it);
         assert(total >= 0);
      }
   }
   
   
}
   
request_handler::request_handler(void):
   total(0), requests_per_round(REQUESTS_PER_ROUND)
{
   assert(remote::world_size != 0);
   all_reqs.resize(remote::world_size);
}

request_handler::~request_handler(void)
{
   assert(total == 0);
   for(size_t i(0); i < remote::world_size; ++i) {
      assert(all_reqs[i].empty());
   }
}
   
}
#endif
