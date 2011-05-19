
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

}

#endif