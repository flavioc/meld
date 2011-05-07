
#ifndef SCHED_NODE_HPP
#define SCHED_NODE_HPP

#include "mem/base.hpp"
#include "db/node.hpp"

namespace sched
{

class stealer; // forward declaration

class thread_node: public db::node
{
private:
   
   friend class stealer;
   
   stealer *owner;
   stealer *worker;
   
public:
   
   inline void set_owner(stealer *_owner) { owner = _owner; }
   
   inline stealer* get_owner(void) { return owner; }
   inline stealer* get_worker(void) { return worker; }
   
   explicit thread_node(const db::node::node_id _id, const db::node::node_id _trans):
      db::node(_id, _trans),
      owner(NULL), worker(NULL)
   {}
   
   virtual ~thread_node(void) { }
};

}

#endif
