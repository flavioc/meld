
#ifndef QUEUE_NODE_HPP
#define QUEUE_NODE_HPP

#include "mem/base.hpp"

namespace queue
{
   
template <class T>
class queue_node: public mem::base
{
public:
   MEM_METHODS(queue_node<T>)

   T data;
   volatile queue_node *next;
};

// same as before, without the volatile
template <class T>
class unsafe_queue_node: public mem::base
{
public:
   MEM_METHODS(unsafe_queue_node<T>)

   virtual size_t mem_size(void) const { return 8; }

   T data;
   unsafe_queue_node *next;
};

}

#endif
