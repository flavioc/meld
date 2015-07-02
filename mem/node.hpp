
#ifndef MEM_NODE_HPP
#define MEM_NODE_HPP

#include "mem/allocator.hpp"
#include "utils/types.hpp"
#include "utils/mutex.hpp"
#include "vm/bitmap_static.hpp"

//#define NODE_ALLOCATOR
//#define USE_REFCOUNT

namespace mem
{

struct node_allocator
{
#ifdef NODE_ALLOCATOR
#define MAX_NODE_ALLOCATOR_SIZE 256
   struct free_size {
      uint16_t size;
      void *list;
   };
#ifdef COMPILED
#define NODE_ALLOCATOR_SIZES COMPILED_NUM_PREDICATES
#else
#define NODE_ALLOCATOR_SIZES 32
#endif
   struct page {
      std::size_t ptr{0};
      std::size_t size{0};
      page *next;
      page *prev;
#ifdef USE_REFCOUNT
      int refcount{0};
      free_size frees[NODE_ALLOCATOR_SIZES];
      uint16_t num_free{0};
#endif
   };
#ifndef USE_REFCOUNT
   uint16_t num_free{0};
   free_size frees[NODE_ALLOCATOR_SIZES];
#endif
   page *first_page{nullptr};
   page *current_page{nullptr};
   utils::mutex mtx;
#ifdef USE_REFCOUNT
   struct object {
      page *page_ptr;
      object *next;
   };
#define ADD_SIZE_OBJ (sizeof(object)-sizeof(object*))
#else
   struct object {
      object *next;
   };
#define ADD_SIZE_OBJ 0
#endif
#define MIN_SIZE_OBJ (sizeof(object*))
#define START_PAGE_SIZE (sizeof(page) * 8)
   static_assert(START_PAGE_SIZE >= sizeof(page), "START_PAGE_SIZE must larger than page size.");

   inline void deallocate_page(page *pg)
   {
      allocator<utils::byte>().deallocate((utils::byte*)pg, pg->size);
   }

   inline void allocate_new_page()
   {
      page *old_page(current_page);
      const size_t new_size(old_page->size * 4);
      current_page = (page*)allocator<utils::byte>().allocate(new_size);
      assert(current_page);
      old_page->prev = current_page;
      current_page->prev = nullptr;
      current_page->next = old_page;
      current_page->size = new_size;
#ifdef USE_REFCOUNT
      current_page->refcount = 0;
      current_page->num_free = 0;
#endif
      current_page->ptr = sizeof(page);
   }

   inline free_size *get_free_size(free_size *frees, uint16_t& num_free, const size_t size, const bool create)
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
#ifndef NDEBUG
      for(size_t i(1); i < num_free; ++i)
         assert(frees[i-1].size < frees[i].size);
#endif
      assert(num_free <= NODE_ALLOCATOR_SIZES);
      return frees + middle;
   }

   inline utils::byte *cast_object_to_ptr(object *o)
   {
      utils::byte *p((utils::byte*)o);

      return p + ADD_SIZE_OBJ;
   }

   inline object* cast_ptr_to_object(utils::byte *p)
   {
      return (object*)(p - ADD_SIZE_OBJ);
   }
#endif

   inline utils::byte *allocate_obj(std::size_t size)
   {
#ifdef NODE_ALLOCATOR
      size = std::max(MIN_SIZE_OBJ, size) + ADD_SIZE_OBJ;
      if(size > MAX_NODE_ALLOCATOR_SIZE)
         return mem::allocator<utils::byte>().allocate(size);
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      assert(current_page);
#ifdef USE_REFCOUNT
      page *pg(current_page);
      while(pg) {
         // start from the top page so that smaller pages are reclaimed faster.
         free_size *f(get_free_size(pg->frees, pg->num_free, size, false));
         if(f && f->list) {
            object *p((object*)f->list);
            f->list = p->next;
            assert(p->page_ptr == pg);
            (pg->refcount)++;
            return cast_object_to_ptr(p);
         }
         pg = pg->next;
      }
#else
      free_size *f(get_free_size(frees, num_free, size, false));
      if(f && f->list) {
         object *p((object*)f->list);
         f->list = p->next;
         return cast_object_to_ptr(p);
      }
#endif
      if(current_page->ptr + size > current_page->size)
         allocate_new_page();
      object *obj = (object*)(((utils::byte*)current_page) + current_page->ptr);
      current_page->ptr += size;
#ifdef USE_REFCOUNT
      obj->page_ptr = current_page;
      (current_page->refcount)++;
#endif
      return cast_object_to_ptr(obj);
#else
      return mem::allocator<utils::byte>().allocate(size);
#endif
   }

   inline void deallocate_obj(utils::byte *p, std::size_t size)
   {
#ifdef NODE_ALLOCATOR
      size = std::max(MIN_SIZE_OBJ, size) + ADD_SIZE_OBJ;
      if(size > MAX_NODE_ALLOCATOR_SIZE)
         return mem::allocator<utils::byte>().deallocate(p, size);
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      object *x(cast_ptr_to_object(p));
#ifdef USE_REFCOUNT
      page *pg(x->page_ptr);

      assert(pg);
      (pg->refcount)--;
      assert(pg->refcount >= 0);
      if(pg->refcount == 0 &&
            (first_page->prev || first_page->next) && // at least one page
            pg != current_page)
      {
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
         deallocate_page(pg);
         assert(first_page);
         assert(current_page);
         return;
      }
#endif
#ifdef USE_REFCOUNT
      free_size *f(get_free_size(pg->frees, pg->num_free, size, true));
#else
      free_size *f(get_free_size(frees, num_free, size, true));
#endif
      assert(f->size == size);
      x->next = (object*)f->list;
      f->list = x;
#else
      mem::allocator<utils::byte>().deallocate(p, size);
#endif
   }

   inline node_allocator() {
#ifdef NODE_ALLOCATOR
      current_page = (page*)allocator<utils::byte>().allocate(START_PAGE_SIZE);
      assert(current_page);
      current_page->prev = current_page->next = nullptr;
      current_page->ptr = sizeof(page);
      current_page->size = START_PAGE_SIZE;
#ifdef USE_REFCOUNT
      current_page->num_free = 0;
      current_page->refcount = 0;
#endif
      first_page = current_page;
#endif
   }

   ~node_allocator() {
#ifdef NODE_ALLOCATOR
      page *next;
      for(page *p(first_page); p; p = next) {
         next = p->next;
         deallocate_page(p);
      }
      first_page = current_page = nullptr;
#endif
   }
};
#undef MIN_SIZE_OBJ
#undef START_PAGE_SIZE
#undef NODE_ALLOCATOR_SIZES
#undef ADD_SIZE_OBJ

};

#endif
