
#ifndef MEM_NODE_HPP
#define MEM_NODE_HPP

#include "mem/allocator.hpp"
#include "utils/types.hpp"
#include "utils/mutex.hpp"

#define NODE_ALLOCATOR

namespace mem
{

struct node_allocator
{
#ifdef NODE_ALLOCATOR
   utils::byte *current_page{nullptr};
   utils::byte *free_list{nullptr};
   std::size_t page_ptr{0};
   std::size_t page_size{0};
   utils::mutex mtx;

   struct object {
      object *next;
      std::size_t size;
   };
#define MIN_SIZE_OBJ sizeof(object)
#define START_PAGE_SIZE (MIN_SIZE_OBJ * 64)

   inline void allocate_new_page()
   {
      page_size *= 4;
      //std::cout << "Page size " << page_size << "\n";
      current_page = allocator<utils::byte>().allocate(page_size);
      page_ptr = 0;
   }

   inline utils::byte *find_free_object(const std::size_t size)
   {
      if(!free_list)
         return nullptr;
      object *p((object*)free_list);
      object *prev{nullptr};

      //int c{0};
      while(p) {
         //c++;
         if(p->size == size) {
            if(prev)
               prev->next = p->next;
            else
               free_list = (utils::byte*)p->next;
            //std::cout << "Took " << c << "\n";
            return (utils::byte*)p;
         } else if(p->size == size * 2) {
            // free object is twice the size of the object we want
            utils::byte *x1((utils::byte*)p);
            utils::byte *x2(x1 + size);
            object *o2((object*)x2);
            o2->next = p->next;
            o2->size = size;
            if(prev)
               prev->next = o2;
            else
               free_list = (utils::byte*)o2;
            //std::cout << "Took/2 " << c << "\n";
            return x1;
         }
         prev = p;
         p = p->next;
      }
      return nullptr;
   }
#endif

   inline utils::byte *allocate_obj(std::size_t size)
   {
#ifdef NODE_ALLOCATOR
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      size = std::max(MIN_SIZE_OBJ, size);
      utils::byte *obj(find_free_object(size));
      if(obj)
         return obj;
      if(page_ptr + size > page_size)
         allocate_new_page();
      obj = current_page + page_ptr;
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
      x->size = size;
      x->next = (object*)free_list;
      free_list = p;
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

};

#endif
