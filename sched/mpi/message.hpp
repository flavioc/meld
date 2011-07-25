
#ifndef SCHED_MPI_MESSAGE_HPP
#define SCHED_MPI_MESSAGE_HPP

#include "conf.hpp"

#include <vector>
#include <ostream>

#include "db/node.hpp"
#include "db/tuple.hpp"
#include "vm/defs.hpp"
#include "mem/base.hpp"
#include "utils/types.hpp"

namespace sched
{
   
#ifdef COMPILE_MPI

class message: public mem::base<message>
{
private:
public:
   
   db::node::node_id id;
   db::simple_tuple *data;

   size_t get_storage_size(void) const
   {
      return sizeof(db::node::node_id) + data->storage_size();
   }
   
   void pack(utils::byte *, const size_t, int *, MPI_Comm) const;
   
   static message *unpack(utils::byte *, const size_t, int *, MPI_Comm);
   static void wipeout(message *msg) { db::simple_tuple::wipeout(msg->data); delete msg; }
   
   explicit message(const db::node::node_id _id,
         const db::simple_tuple *_data):
      id(_id), data((db::simple_tuple*)_data)
   {
   }
   
   explicit message(void) {} // for serialization purposes
   
   ~message(void)
   {
   }
};

std::ostream& operator<<(std::ostream& cout, const message& msg);

typedef std::vector<message*, mem::allocator<message*> > list_messages;

class message_set: public mem::base<message_set>
{
private:
   static const size_t INITIAL_MESSAGE_SIZE = sizeof(unsigned short int); /* size of number of messages */
      
   list_messages lst;
   size_t total_size;
   
public:
   
   void add(message *msg)
   {
      lst.push_back(msg);
      assert(total_size > 0);
      total_size += msg->get_storage_size();
      assert(total_size > lst.size());
   }
   
   inline size_t size(void) const { return lst.size(); }
   
   inline bool empty(void) const
   {
      if(lst.empty())
         assert(total_size == INITIAL_MESSAGE_SIZE);
      return lst.empty();
   }
   
   size_t get_storage_size(void) const { return total_size; }
   
   void pack(utils::byte *, const size_t, MPI_Comm) const;
   
   inline list_messages::const_iterator begin(void) const { return lst.begin(); }
   inline list_messages::const_iterator end(void) const { return lst.end(); }
   
   static message_set* unpack(utils::byte *, const size_t, MPI_Comm);
   
   void wipeout(void)
   {
      for(list_messages::iterator it(lst.begin()); it != lst.end(); ++it)
         message::wipeout(*it);
      lst.clear();
      total_size = INITIAL_MESSAGE_SIZE;
   }
   
   explicit message_set(void): total_size(INITIAL_MESSAGE_SIZE)
   {
      assert(total_size > 0);
   }
   
   ~message_set(void) {}
};

#endif

}

BOOST_CLASS_TRACKING(sched::message, boost::serialization::track_never)

#endif
