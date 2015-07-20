
#ifndef MEM_BIGCHUNK_HPP
#define MEM_BIGCHUNK_HPP

#include <map>

#include "utils/types.hpp"
#include "mem/stat.hpp"

namespace mem
{

struct bigpage {
   bigpage *prev;
   bigpage *next;
};

struct bigchunk {
   bigpage *first;
   bigpage *current;
   size_t size;
   utils::byte *ptr, *end;
#define INITIAL_BIGCHUNK_SIZE 4096

   inline std::pair<utils::byte*, utils::byte*> fetch(const size_t hint_size, const size_t unit)
   {
      if(ptr + unit <= end) {
         utils::byte *next_ptr(std::min(end, ptr + hint_size));
         const auto ret(std::make_pair(ptr, next_ptr));
         assert(ptr + unit <= end);
         assert(next_ptr <= end);
         ptr = next_ptr;
         return ret;
      }
      // allocate new
      size *= 2;
      while(size - sizeof(bigpage) < 8 * hint_size) // must fit 8 objects of this size.
         size *= 2;
      assert(hint_size + sizeof(bigpage) <= size);
      //std::cout << "Size " << size << "\n";
      utils::byte *chunk(new utils::byte[size]);
      register_malloc(size);
      assert(chunk);
      bigpage *page((bigpage*)chunk);
      page->prev = current;
      page->next = nullptr;
      current->next = page;
      current = page;
      ptr = chunk + sizeof(bigpage);
      end = chunk + size;
      utils::byte *next_ptr(ptr + hint_size);
      assert(next_ptr <= end);
      const auto ret(std::make_pair(ptr, next_ptr));
      ptr = next_ptr;
      return ret;
   }

   inline void create()
   {
      size = INITIAL_BIGCHUNK_SIZE;
      utils::byte *p(new utils::byte[size]);
      register_malloc(size);
      first = current = (bigpage*)p;
      first->next = first->prev = nullptr;
      ptr = p + sizeof(bigchunk);
      end = p + size;
   }
};

}

#endif
