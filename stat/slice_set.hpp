
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
#include "utils/macros.hpp"

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
   
   inline void
   write_header(std::ofstream& out, const std::string& header, vm::all *all) const
   {
      utils::csv_line line;

      line.set_header();

      line << header;

      for(size_t i(0); i < all->NUM_THREADS; ++i)
         line << (std::string("thread") + utils::to_string<size_t>(i));

      line.print(out);
   }
     
   template <class T>
   void write_general(const std::string& file, const std::string& title,
      vm::all *all, T fun) const
   {
      std::ofstream out(file.c_str(), std::ios_base::out | std::ios_base::trunc);
      
      write_header(out, title, all);
      
      std::vector<iter> iters(all->NUM_THREADS, iter());

       assert(slices.size() == all->NUM_THREADS);

      for(size_t i(0); i < all->NUM_THREADS; ++i) {
           assert(num_slices == slices[i].size());
         iters[i] = slices[i].begin();
       }
         
      for(size_t slice(0); slice < num_slices; ++slice) {
         utils::csv_line line;
         line << slice * SLICE_PERIOD;
         for(size_t proc(0); proc < all->NUM_THREADS; ++proc) {
            assert(iters[proc] != slices[proc].end());
            const statistics::slice& sl(*iters[proc]);
            line << fun(sl);
            iters[proc]++;
         }
         line.print(out);
      }
   }

   
public:
   
   void write(const std::string&, vm::all *) const;
   
   void beat(vm::all *);
   
   explicit slice_set(const size_t);
   
   virtual ~slice_set(void) {}
};

}

#endif
