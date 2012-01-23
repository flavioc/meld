
#ifndef MEM_CENTER_HPP
#define MEM_CENTER_HPP

#include "conf.hpp"
#include "mem/stat.hpp"
#include "mem/thread.hpp"

#ifdef ALLOCATOR_ASSERT
extern boost::mutex allocator_mtx;
extern std::tr1::unordered_set<void*> mem_set; 
#endif

namespace mem
{

class center
{
private:
public:
   static inline void* allocate(size_t cnt, size_t sz)
   {
      void *p;
      
      register_allocation(cnt, sz);
      
      if(USE_ALLOCATOR)
         p = get_pool()->allocate(cnt * sz);
      else
         p = ::operator new(cnt * sz);
         
#ifdef ALLOCATOR_ASSERT
      allocator_mtx.lock();
      assert(mem_set.find(p) == mem_set.end());
      mem_set.insert(p);
      allocator_mtx.unlock();
#endif

      return p;
   }
   
   static inline void deallocate(void *p, size_t cnt, size_t sz)
   {
#ifdef ALLOCATOR_ASSERT
      allocator_mtx.lock();
      assert(mem_set.find(p) != mem_set.end()); 
      mem_set.erase(p);
      allocator_mtx.unlock();
#endif

      register_deallocation(cnt, sz);

      if(USE_ALLOCATOR)
         get_pool()->deallocate(p, cnt * sz);
      else
         ::operator delete(p);
   }
   
};

}

#endif
