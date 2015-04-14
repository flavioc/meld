
#ifndef MEM_NODE_HPP
#define MEM_NODE_HPP

#include "mem/allocator.hpp"
#include "utils/types.hpp"
#include "utils/mutex.hpp"
#include "vm/bitmap_static.hpp"
#ifdef COMPILED
#include COMPILED_HEADER
#endif

#define NODE_ALLOCATOR

namespace mem
{

struct node_allocator
{
#ifdef NODE_ALLOCATOR
   utils::byte *current_page{nullptr};
   std::size_t page_ptr{0};
   std::size_t page_size{0};
   utils::mutex mtx;
   struct object {
      object *next;
   };
   struct free_size {
      std::size_t size;
      object *list;
   };
#ifdef COMPILED
#define NODE_ALLOCATOR_SIZES COMPILED_NUM_PREDICATES
#else
#define NODE_ALLOCATOR_SIZES 32
#endif
   std::size_t num_free{0};
   free_size frees[NODE_ALLOCATOR_SIZES];

#define MIN_SIZE_OBJ sizeof(object)
#define START_PAGE_SIZE (MIN_SIZE_OBJ * 64)

   inline void allocate_new_page()
   {
      page_size *= 4;
      current_page = allocator<utils::byte>().allocate(page_size);
      page_ptr = 0;
   }

   inline free_size *get_free_size(const size_t size)
   {
      int left(0);
      int right(static_cast<int>(num_free) - 1);

      while(left <= right) {
         std::size_t middle(left + (right - left)/2);
         const std::size_t found(frees[middle].size);

         if(found == size)
            return frees + middle;
         else if(left == right) {
            // need to add it here.
            if(size > found)
               middle++;
//            std::cout << "Shifting for " << size << " middle: " << middle << "/" << num_free << " " << (num_free - middle) << "\n";
            memmove(frees + middle + 1, frees + middle, (num_free - middle) * sizeof(free_size));
            frees[middle].size = size;
            frees[middle].list = nullptr;
            num_free++;
            assert(num_free <= NODE_ALLOCATOR_SIZES);
            return frees + middle;
         } else if(found < size)
            left = middle + 1;
         else
            right = middle - 1;
      }
      // empty array.
      frees[0].size = size;
      frees[0].list = nullptr;
      num_free++;
      return frees;
   }

#endif

   inline utils::byte *allocate_obj(std::size_t size)
   {
#ifdef NODE_ALLOCATOR
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      size = std::max(MIN_SIZE_OBJ, size);
      int left(0);
      int right(static_cast<int>(num_free) - 1);

      while(left <= right) {
         int middle{left + (right - left)/2};
         std::size_t found(frees[middle].size);

         if(found == size) {
            if(frees[middle].list) {
               object *p(frees[middle].list);
               frees[middle].list = p->next;
               return (utils::byte*)p;
            }
            break;
         } else if(found < size)
            left = middle + 1;
         else
            right = middle - 1;
      }
      if(page_ptr + size > page_size)
         allocate_new_page();
      utils::byte *obj = current_page + page_ptr;
      page_ptr += size;
      return obj;
#else
      return mem::allocator<utils::byte>().allocate(size);
#endif
   }

   inline void deallocate_obj(utils::byte *p, std::size_t size)
   {
#ifdef NODE_ALLOCATOR
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      size = std::max(MIN_SIZE_OBJ, size);
      object *x((object*)p);
      free_size *f(get_free_size(size));
      x->next = f->list;
      f->list = x;
#else
      mem::allocator<utils::byte>().deallocate(p, size);
#endif
   }

   inline node_allocator() {
#ifdef NODE_ALLOCATOR
      current_page = allocator<utils::byte>().allocate(START_PAGE_SIZE);
      page_size = START_PAGE_SIZE;
#endif
   }
};
#undef MIN_SIZE_OBJ
#undef START_PAGE_SIZE
#undef NODE_ALLOCATOR_SIZES

};

#endif
