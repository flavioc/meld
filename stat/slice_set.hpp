
#ifndef STAT_SLICE_SET
#define STAT_SLICE_SET

#include <vector>
#include <list>
#include <string>
#include <functional>

#include "mem/allocator.hpp"
#include "stat/slice.hpp"
#include "vm/defs.hpp"
#include "vm/all.hpp"

namespace statistics
{
   
class slice_set
{
private:
   
   size_t num_slices;
   
   typedef std::list<slice> list_slices;
   typedef list_slices::const_iterator iter;
   
   typedef std::vector<list_slices> vector_slices;
   
   vector_slices slices;
   
   void beat_thread(const vm::process_id, slice&, vm::all *);
   
   typedef  void (slice::*print_fn)(utils::csv_line&) const;
   
   void write_general(const std::string&, const std::string&, print_fn, vm::all *) const;
   
public:
   
   void write(const std::string&, vm::all *) const;
   
   void beat(vm::all *);
   
   explicit slice_set(const size_t);
   
   virtual ~slice_set(void) {}
};

}

#endif
