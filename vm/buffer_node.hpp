
#ifndef VM_BUFFER_NODE_HPP
#define VM_BUFFER_NODE_HPP

#include <list>

#include "vm/tuple.hpp"
#include "mem/allocator.hpp"

namespace vm
{

struct buffer_item
{
   vm::predicate *pred{nullptr};
   vm::tuple_list ls;
};

struct buffer_node {
   std::list<buffer_item, mem::allocator<buffer_item>> ls;

   inline size_t size() const
   {
      size_t count(0);
      for(buffer_item i : ls) {
         count += i.ls.get_size();
      }
      return count;
   }
};
}

#endif
