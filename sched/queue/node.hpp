
#ifndef SCHED_QUEUE_NODE_HPP
#define SCHED_QUEUE_NODE_HPP

#include "mem/base.hpp"

namespace sched
{
   
template <class T>
class queue_node: public mem::base< queue_node<T> >
{
public:
   T data;
   volatile queue_node *next;
};

// same as before, without the volatile
template <class T>
class unsafe_queue_node: public mem::base< unsafe_queue_node<T> >
{
public:
   T data;
   unsafe_queue_node *next;
};

}

#endif