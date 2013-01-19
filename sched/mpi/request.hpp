
#ifndef SCHED_MPI_REQUEST_HPP
#define SCHED_MPI_REQUEST_HPP

#include "conf.hpp"

#ifdef COMPILE_MPI

#include <utility>
#include <list>
#include <boost/mpi/request.hpp>

#include "vm/all.hpp"
#include "mem/allocator.hpp"
#include "process/remote.hpp"

namespace sched
{

struct req_obj {
   MPI_Request mpi_req;
   utils::byte *mem;
   size_t mem_size;
};

typedef std::list<req_obj, mem::allocator<req_obj> > list_reqs;

class request_handler
{
private:
   
   std::vector<list_reqs, mem::allocator<list_reqs> > all_reqs;
   int total;
   size_t requests_per_round;
   
   void clear(void);
   
public:
   
   inline bool empty(void) const { return total == 0; }
   
   void flush(const bool, vm::all *);
   
   inline void add_request(const process::remote* rem, req_obj& req) {
      assert(rem->get_rank() < (int)all_reqs.size());
      ++total;
      all_reqs[rem->get_rank()].push_back(req);
   }
   
   explicit request_handler(void);
   ~request_handler(void);
};

}

#endif

#endif

