
#ifndef UTILS_TREE_BARRIER_HPP
#define UTILS_TREE_BARRIER_HPP

#include "utils/atomic.hpp"
#include "utils/utils.hpp"

#include <stdio.h>

namespace utils
{

class tree_barrier
{
private:

   class inner_node
   {
   private:
      
      tree_barrier *outer;
      inner_node *parent;
      size_t count;
      utils::atomic<size_t> children_count;
      bool thread_sense;
      const size_t id;
      
   public:
      
      void set_count(const size_t n)
      {
         count = n;
         children_count = n;
      }
      
      inline void wait(void)
      {
         bool my_sense(thread_sense);
         
         if(id == 0)
            assert(parent == NULL);
         
         while(children_count > 0) {}
         
         children_count = count;
         
         if(parent != NULL) {
            // not root
            parent->children_count--;
            while(outer->sense != my_sense) {}
         } else {
            // root
            outer->sense = !outer->sense;
            assert(outer->sense == my_sense);
         }
         
         thread_sense = !my_sense;
      }
      
      explicit inner_node(tree_barrier *tb, inner_node *_parent, const size_t _count, const size_t _id):
         outer(tb), parent(_parent),
         count(_count),
         children_count(_count),
         thread_sense(!(outer->sense)),
         id(_id)
      {
         if(id == 0)
            assert(parent == NULL);
      }
   };

   
   const size_t size;
   const size_t radix;
   size_t num_nodes;
   inner_node **node;
   
   volatile bool sense;   
   
   bool build(inner_node *parent, const size_t depth)
   {
      const size_t this_id(num_nodes);
      
      if(num_nodes == size)
         return false;
      
      ++num_nodes;

      if(depth == 0)
         node[this_id] = new inner_node(this, parent, 0, this_id);
      else {
         inner_node *my_node(new inner_node(this, parent, radix, this_id));
         node[this_id] = my_node;
         
         size_t num_children(0);
         for(size_t i(0); i < radix; ++i) {
            if(build(my_node, depth - 1))
               num_children++;
         }
         
         if(num_children != radix)
            my_node->set_count(num_children);
      }
      
      return true;
   }
   
public:
   
   inline void wait(const size_t id)
   {
      node[id]->wait();
   }
   
   explicit tree_barrier(size_t s, const size_t _radix = 2):
      size(s), radix(_radix)
   {
      typedef inner_node* node_ptr;
      node = new node_ptr[size];
      
      size_t depth(0);
      while(s > 1) {
         depth++;
         s = s / radix;
      }
      
      sense = false;
      
      num_nodes = 0;
      build(NULL, depth);
   }
};

}

#endif
