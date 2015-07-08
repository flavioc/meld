
#ifndef MEM_NODE_HPP
#define MEM_NODE_HPP

#include "mem/allocator.hpp"
#include "utils/types.hpp"
#include "utils/mutex.hpp"
#include "vm/bitmap_static.hpp"

//#define NODE_ALLOCATOR
//#define NODE_REFCOUNT

#ifndef POOL_ALLOCATOR
#ifdef NODE_ALLOCATOR
#undef NODE_ALLOCATOR
#endif
#ifdef NODE_REFCOUNT
#undef NODE_REFCOUNT
#endif
#endif

namespace mem
{

struct node_allocator
{
#ifdef NODE_ALLOCATOR
#define MAX_NODE_ALLOCATOR_SIZE 1024
   struct free_size {
      uint32_t size;
      void *list;
   };
#ifdef COMPILED
#define NODE_ALLOCATOR_SIZES (COMPILED_NUM_PREDICATES)
#else
#define NODE_ALLOCATOR_SIZES 32
#endif
   struct page {
      std::size_t ptr{0};
      std::size_t size{0};
      page *next;
      page *prev;
#ifdef NODE_REFCOUNT
      bool dynamic;
      int refcount{0};
      free_size frees[NODE_ALLOCATOR_SIZES];
      uint16_t num_free{0};
#endif
   };
#ifndef NODE_REFCOUNT
   bool dynamic;
   uint16_t num_free{0};
   free_size frees[NODE_ALLOCATOR_SIZES];
#endif
   page *first_page{nullptr};
   page *current_page{nullptr};
   utils::mutex mtx;
#ifdef NODE_REFCOUNT
   struct object {
      page *page_ptr;
      object *next;
   };
#define NODE_ADD_SIZE_OBJ (sizeof(object)-sizeof(object*))
#else
   struct object {
      object *next;
   };
#define NODE_ADD_SIZE_OBJ 0
#endif
#define NODE_MIN_SIZE_OBJ (sizeof(object*))
#define NODE_START_PAGE_SIZE (2048)
   static_assert(NODE_START_PAGE_SIZE >= sizeof(page), "NODE_START_PAGE_SIZE must larger than page size.");

   inline void deallocate_page(page *pg)
   {
      allocator<utils::byte>().deallocate((utils::byte*)pg, pg->size);
   }

   inline void allocate_new_page(const size_t required)
   {
      page *old_page(current_page);
      size_t new_size(old_page->size * 2);
      while(new_size - sizeof(page) < required)
         new_size *= 2;
      current_page = (page*)allocator<utils::byte>().allocate(new_size);
      assert(current_page);
      old_page->prev = current_page;
      current_page->prev = nullptr;
      current_page->next = old_page;
      current_page->size = new_size;
#ifdef NODE_REFCOUNT
      current_page->dynamic = false;
      current_page->refcount = 0;
      current_page->num_free = 0;
#endif
      current_page->ptr = sizeof(page);
   }

   inline free_size *get_free_size(free_size *frees, uint16_t& num_free, const size_t size, const bool create, bool& dynamic)
   {
      int left(0);
      int right(static_cast<int>(num_free) - 1);
      int middle(0);
      free_size *initial_frees(frees);
      if(dynamic)
         frees = (free_size*)frees[0].list;
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
      if(middle < num_free && size > frees[middle].size)
         middle++;
      // need to add it here.
      assert(middle <= num_free + 1);
      assert(middle >= 0);
      if(num_free == NODE_ALLOCATOR_SIZES && !dynamic) {
         dynamic = true;
         const size_t newsize(NODE_ALLOCATOR_SIZES * 2);
         free_size *n((free_size*)mem::allocator<free_size>().allocate(newsize));
         memcpy(n, frees, num_free*sizeof(free_size));
         frees[0].list = n;
         frees[0].size = newsize;
         frees = n;
      } else if(dynamic && initial_frees[0].size == num_free) {
         free_size *n((free_size*)mem::allocator<free_size>().allocate(initial_frees[0].size * 2));
         memcpy(n, frees, num_free*sizeof(free_size));
         mem::allocator<free_size>().deallocate(frees, initial_frees[0].size);
         initial_frees[0].list = n;
         initial_frees[0].size *= 2;
         frees = n;
      }
      assert((!dynamic && num_free < NODE_ALLOCATOR_SIZES) || (dynamic && num_free < initial_frees[0].size));
      if(middle < num_free)
         memmove(frees + middle + 1, frees + middle, (num_free - middle) * sizeof(free_size));
      frees[middle].size = size;
      frees[middle].list = nullptr;
      num_free++;
#ifndef NDEBUG
      for(size_t i(1); i < num_free; ++i)
         assert(frees[i-1].size < frees[i].size);
#endif
      return frees + middle;
   }

   inline utils::byte *cast_object_to_ptr(object *o)
   {
      utils::byte *p((utils::byte*)o);

      return p + NODE_ADD_SIZE_OBJ;
   }

   inline object* cast_ptr_to_object(utils::byte *p)
   {
      return (object*)(p - NODE_ADD_SIZE_OBJ);
   }
#endif

   inline utils::byte *allocate_obj(std::size_t size)
   {
#ifdef NODE_ALLOCATOR
      size = std::max(NODE_MIN_SIZE_OBJ, size) + NODE_ADD_SIZE_OBJ;
      if(size > MAX_NODE_ALLOCATOR_SIZE)
         return mem::allocator<utils::byte>().allocate(size);
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      assert(current_page);
#ifdef NODE_REFCOUNT
      page *pg(current_page);
      while(pg) {
         // start from the top page so that smaller pages are reclaimed faster.
         free_size *f(get_free_size(pg->frees, pg->num_free, size, false, pg->dynamic));
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
      free_size *f(get_free_size(frees, num_free, size, false, dynamic));
      if(f && f->list) {
         object *p((object*)f->list);
         f->list = p->next;
         return cast_object_to_ptr(p);
      }
#endif
      if(current_page->ptr + size > current_page->size)
         allocate_new_page(size);
      object *obj = (object*)(((utils::byte*)current_page) + current_page->ptr);
      current_page->ptr += size;
#ifdef NODE_REFCOUNT
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
      size = std::max(NODE_MIN_SIZE_OBJ, size) + NODE_ADD_SIZE_OBJ;
      if(size > MAX_NODE_ALLOCATOR_SIZE)
         return mem::allocator<utils::byte>().deallocate(p, size);
      MUTEX_LOCK_GUARD(mtx, allocator_lock);
      object *x(cast_ptr_to_object(p));
#ifdef NODE_REFCOUNT
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
#ifdef NODE_REFCOUNT
      free_size *f(get_free_size(pg->frees, pg->num_free, size, true, pg->dynamic));
#else
      free_size *f(get_free_size(frees, num_free, size, true, dynamic));
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
      current_page = (page*)allocator<utils::byte>().allocate(NODE_START_PAGE_SIZE);
      assert(current_page);
      current_page->prev = current_page->next = nullptr;
      current_page->ptr = sizeof(page);
      current_page->size = NODE_START_PAGE_SIZE;
#ifdef NODE_REFCOUNT
      current_page->num_free = 0;
      current_page->refcount = 0;
#else
      dynamic = false;
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
#undef NODE_MIN_SIZE_OBJ
#undef NODE_START_PAGE_SIZE
#undef NODE_ALLOCATOR_SIZES
#undef NODE_ADD_SIZE_OBJ

};

#endif
