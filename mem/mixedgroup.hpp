
#ifndef MEM_MIXEDGROUP_HPP
#define MEM_MIXEDGROUP_HPP

#include <stdint.h>
#include <cstdlib>
#include "utils/types.hpp"
#include "mem/stat.hpp"

namespace mem
{

struct mixedgroup
{
   struct free_size {
      uint32_t size;
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
      int middle(0);
#if 0
      std::cout << "\n";

      for(size_t i(0); i < num_free; ++i)
         std::cout << frees[i].size << " ";
      std::cout << "\n";
#endif

      while(left <= right) {
         middle = left + (right - left)/2;
#if 0
         std::cout << "l: " << left << " r: " << right << " m: " << middle << "\n";
#endif
         assert(middle < num_free);
         assert(middle >= 0);
         assert(left < num_free);
         assert(right >= 0);

         if(frees[middle].size == size)
            return frees + middle;
         else if(left == right)
            break;
         else if(frees[middle].size < size)
            left = middle + 1;
         else
            right = middle;
      }
      if(!create)
         return nullptr;
#if 0
      std::cout << "l: " << left << " r: " << right << " m: " << middle << "\n";
#endif
#ifndef NDEBUG
      for(size_t i(0); i < num_free; ++i)
         assert(frees[i].size != size);
#endif
      if(size > frees[middle].size)
         middle++;
      // need to add it here.
      assert(middle <= num_free + 1);
      assert(middle >= 0);
      if(middle < num_free)
         memmove(frees + middle + 1, frees + middle, (num_free - middle) * sizeof(free_size));
      frees[middle].size = size;
      frees[middle].list = nullptr;
      num_free++;
      if(num_free > ALLOCATOR_FREE_SIZE)
         abort();
#ifndef NDEBUG
      for(size_t i(1); i < num_free; ++i)
         assert(frees[i-1].size < frees[i].size);
#endif
      assert(num_free <= ALLOCATOR_FREE_SIZE);
      return frees + middle;
   }

   inline page* allocate_new_page(size_t size, const size_t atleast)
   {
      while(size + sizeof(page) < 8 * atleast)
         size *= 2;
      utils::byte *data = new utils::byte[size];
      register_malloc(size);
      assert(size > sizeof(page));
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
         allocate_new_page(current_page->size * 2, size);
      assert(current_page->ptr + size <= current_page->size);
      object *obj = (object*)(((utils::byte*)current_page) + current_page->ptr);
      current_page->ptr = current_page->ptr + size;
      return cast_object_to_ptr(obj);
   }

   inline void deallocate(void *p, size_t size)
   {
      size = std::max(MIN_SIZE_OBJ, size);
      object *x(cast_ptr_to_object((utils::byte*)p));
      free_size *f(get_free_size(size, true));
      assert(f);
      assert(f->size == size);
      x->next = (object*)f->list;
      f->list = x;
   }

   inline void create(const size_t factor) {
      first_page = allocate_new_page(START_PAGE_SIZE * factor, 0);
      num_free = 0;
   }
};

}

#endif
