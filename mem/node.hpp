
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
   struct free_size {
      uint8_t size;
      void *list;
   };
#ifdef COMPILED
#define NODE_ALLOCATOR_SIZES COMPILED_NUM_PREDICATES
#else
#define NODE_ALLOCATOR_SIZES 32
#endif
   struct page {
      uint16_t refcount{0};
      page *next;
      page *prev;
      std::size_t ptr{0};
      std::size_t size{0};
      uint8_t num_free{0};
      free_size frees[NODE_ALLOCATOR_SIZES];
   };
   page *first_page{nullptr};
   page *current_page{nullptr};
   utils::mutex mtx;
   struct object {
      page *page_ptr;
      object *next;
   };
#define ADD_SIZE_OBJ sizeof(void*)
#define MIN_SIZE_OBJ sizeof(object)
#define START_PAGE_SIZE (MIN_SIZE_OBJ * 64)

   inline void allocate_new_page()
   {
      page *old_page(current_page);
      const size_t new_size(old_page->size * 4);
      current_page = (page*)allocator<utils::byte>().allocate(new_size);
      old_page->prev = current_page;
      current_page->next = old_page;
      current_page->size = new_size;
      current_page->refcount = 0;
      current_page->num_free = 0;
      current_page->ptr = sizeof(page);
   }

   inline free_size *get_free_size(free_size *frees, uint8_t& num_free, const size_t size, const bool create)
   {
      int left(0);
      int right(static_cast<int>(num_free) - 1);

      while(left <= right) {
         std::size_t middle(left + (right - left)/2);
         const std::size_t found(frees[middle].size);

         if(found == size)
            return frees + middle;
         else if(left == right) {
            if(!create)
               return nullptr;
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
      if(!create)
         return nullptr;
      // empty array.
      frees[0].size = size;
      frees[0].list = nullptr;
      num_free++;
      return frees;
   }

   inline utils::byte *cast_object_to_ptr(object *o)
   {
      utils::byte *p((utils::byte*)o);

      return p + sizeof(void*);
   }

   inline object* cast_ptr_to_object(utils::byte *p)
   {
      return (object*)(p - sizeof(void*));
   }
#endif

   inline utils::byte *allocate_obj(std::size_t size)
   {
#ifdef NODE_ALLOCATOR
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      size = std::max(MIN_SIZE_OBJ, size + ADD_SIZE_OBJ);
      page *pg(current_page);
      while(pg) {
         // start from the top page so that smaller pages are reclaimed faster.
         free_size *f(get_free_size(pg->frees, pg->num_free, size, false));
         if(f && f->list) {
            object *p((object*)f->list);
            f->list = p->next;
            p->page_ptr->refcount++;
            return cast_object_to_ptr(p);
         }
         pg = pg->next;
      }
      if(current_page->ptr + size > current_page->size)
         allocate_new_page();
      object *obj = (object*)(((utils::byte*)current_page) + current_page->ptr);
      current_page->ptr += size;
      obj->page_ptr = current_page;
      obj->page_ptr->refcount++;
      return cast_object_to_ptr(obj);
#else
      return mem::allocator<utils::byte>().allocate(size);
#endif
   }

   inline void deallocate_obj(utils::byte *p, std::size_t size)
   {
#ifdef NODE_ALLOCATOR
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      size = std::max(MIN_SIZE_OBJ, size + ADD_SIZE_OBJ);
      object *x(cast_ptr_to_object(p));
      page *pg(x->page_ptr);
      if(--(pg->refcount) == 0 && current_page != first_page) {
         page *prev(pg->prev);
         page *next(pg->next);
         if(prev)
            prev->next = next;
         if(next)
            next->prev = prev;
         if(pg == first_page)
            first_page = prev;
         if(pg == current_page)
            current_page = next;
         return;
      }
      free_size *f(get_free_size(pg->frees, pg->num_free, size, true));
      x->next = (object*)f->list;
      f->list = x;
#else
      mem::allocator<utils::byte>().deallocate(p, size);
#endif
   }

   inline node_allocator() {
#ifdef NODE_ALLOCATOR
      current_page = (page*)allocator<utils::byte>().allocate(START_PAGE_SIZE);
      current_page->refcount = 0;
      current_page->prev = current_page->next = nullptr;
      current_page->ptr = sizeof(page);
      current_page->size = START_PAGE_SIZE;
      current_page->num_free = 0;
      first_page = current_page;
#endif
   }
};
#undef MIN_SIZE_OBJ
#undef START_PAGE_SIZE
#undef NODE_ALLOCATOR_SIZES

};

#endif
