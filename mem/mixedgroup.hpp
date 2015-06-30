
#ifndef MEM_MIXEDGROUP_HPP
#define MEM_MIXEDGROUP_HPP

#include <stdint.h>
#include <cstdlib>
#include "utils/types.hpp"

namespace mem
{

struct mixedgroup
{
   struct free_size {
      uint16_t size;
      void *list;
   };
   struct page {
      size_t ptr{0};
      size_t size{0};
      page *next;
      page *prev;
   };
   uint16_t num_free{0};
#define ALLOCATOR_FREE_SIZE 32
   free_size frees[ALLOCATOR_FREE_SIZE];
   page *first_page{nullptr};
   page *current_page{nullptr};
   struct object {
      object *next;
   };
#define MIN_SIZE_OBJ (sizeof(object*))
#define START_PAGE_SIZE (sizeof(page) * 8)
   static_assert(START_PAGE_SIZE >= sizeof(page), "START_PAGE_SIZE must larger than page size.");

   inline free_size *get_free_size(const size_t size, const bool create)
   {
      int left(0);
      int right(static_cast<int>(num_free) - 1);

      while(left <= right) {
         int middle(left + (right - left)/2);
         assert(middle < num_free);
         assert(middle >= 0);
         assert(left < num_free);
         assert(right >= 0);
         const std::size_t found(frees[middle].size);

         if(found == size)
            return frees + middle;
         else if(left == right) {
            if(!create)
               return nullptr;
            // need to add it here.
            if(size > found)
               middle++;
            assert(middle <= num_free);
            assert(middle >= 0);
            memmove(frees + middle + 1, frees + middle, (num_free - middle) * sizeof(free_size));
            frees[middle].size = size;
            frees[middle].list = nullptr;
            assert(num_free == middle || frees[middle+1].size > frees[middle].size);
            assert(middle == 0 || frees[middle-1].size < frees[middle].size);
            num_free++;
            assert(num_free <= ALLOCATOR_FREE_SIZE);
            return frees + middle;
         } else if(found < size)
            left = middle + 1;
         else
            right = middle - 1;
      }
      if(!create)
         return nullptr;
      frees[num_free].size = size;
      frees[num_free].list = nullptr;
      num_free++;
      assert(num_free <= ALLOCATOR_FREE_SIZE);
      return frees + (num_free-1);
   }

   inline page* allocate_new_page(const size_t size)
   {
      utils::byte *data = new utils::byte[size];
      page *prev(current_page);
      current_page = (page*)data;
      current_page->size = size;
      current_page->ptr = sizeof(page);
      current_page->prev = prev;
      current_page->next = nullptr;
      return current_page;
   }

   inline utils::byte *cast_object_to_ptr(object *o)
   {
      utils::byte *p((utils::byte*)o);
      return p;
   }

   inline object* cast_ptr_to_object(utils::byte *p)
   {
      return (object*)p;
   }

   inline void *allocate(size_t size) {
      size = std::max(MIN_SIZE_OBJ, size);
      free_size *f(get_free_size(size, false));
      if(f && f->list) {
         object *p((object*)f->list);
         f->list = p->next;
         return cast_object_to_ptr(p);
      }
      if(current_page->ptr + size > current_page->size)
         allocate_new_page(current_page->size * 2);
      object *obj = (object*)(((utils::byte*)current_page) + current_page->ptr);
      current_page->ptr += size;
      return cast_object_to_ptr(obj);
   }

   inline void deallocate(void *p, size_t size)
   {
      size = std::max(MIN_SIZE_OBJ, size);
      object *x(cast_ptr_to_object((utils::byte*)p));
      free_size *f(get_free_size(size, true));
      assert(f->size == size);
      x->next = (object*)f->list;
      f->list = x;
   }

   inline void create(const size_t factor) {
      first_page = allocate_new_page(START_PAGE_SIZE * factor);
      num_free = 0;
   }
};

}

#endif
