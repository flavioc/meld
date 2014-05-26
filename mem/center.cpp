
#include <tr1/unordered_set>

#include "mem/center.hpp"

#ifdef ALLOCATOR_ASSERT
extern boost::mutex allocator_mtx;
extern std::tr1::unordered_set<void*> mem_set; 
#endif

#ifdef TRACK_MEMORY
size_t bytes_used(0);
#endif

namespace mem
{

void*
center::allocate(size_t cnt, size_t sz)
{
   void *p;
   
   register_allocation(cnt, sz);
#ifdef TRACK_MEMORY
   bytes_used += cnt * sz;
#endif
   
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

void
center::deallocate(void *p, size_t cnt, size_t sz)
{
#ifdef ALLOCATOR_ASSERT
   allocator_mtx.lock();
   assert(mem_set.find(p) != mem_set.end()); 
   mem_set.erase(p);
   allocator_mtx.unlock();
#endif

   register_deallocation(cnt, sz);
#ifdef TRACK_MEMORY
   bytes_used -= cnt * sz;
#endif

   if(USE_ALLOCATOR)
      get_pool()->deallocate(p, cnt * sz);
   else
      ::operator delete(p);
}

}
