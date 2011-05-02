
#ifndef PROCESS_MESSAGE_HPP
#define PROCESS_MESSAGE_HPP

#include "conf.hpp"

#ifdef COMPILE_MPI
#include <boost/serialization/serialization.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#endif

#include <vector>
#include <ostream>

#include "db/node.hpp"
#include "db/tuple.hpp"
#include "vm/defs.hpp"
#include "mem/base.hpp"

namespace process
{

class message: public mem::base<message>
{
private:

#ifdef COMPILE_MPI
   friend class boost::serialization::access;
   
   inline void save(boost::mpi::packed_oarchive& ar, const unsigned int) const
   {
      ar & id;
      ar & *data;
   }
   
   inline void load(boost::mpi::packed_iarchive& ar, const unsigned int)
   {
      data = new db::simple_tuple();
      
      ar & id;
      ar & *data;
   }
   
   BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

public:
   
   db::node::node_id id;
   db::simple_tuple *data;
   
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
#ifdef COMPILE_MPI
   friend class boost::serialization::access;

   inline void save(boost::mpi::packed_oarchive& ar, const unsigned int) const
   {
      ar & lst;
      ar & source;
   }

   inline void load(boost::mpi::packed_iarchive& ar, const unsigned int)
   {
      ar & lst;
      ar & source;
   }

   BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif
   
public:
   list_messages lst;
   vm::process_id source;
   
   void add(message *msg) { lst.push_back(msg); }
   
   inline const size_t size(void) const { return lst.size(); }
   inline const bool empty(void) const { return lst.empty(); }
   
   inline list_messages::const_iterator begin(void) const { return lst.begin(); }
   inline list_messages::const_iterator end(void) const { return lst.end(); }
   
   void wipeout(void) {
      for(list_messages::iterator it(lst.begin()); it != lst.end(); ++it)
         message::wipeout(*it);
      lst.clear();
   }
   
   explicit message_set(const vm::process_id _source): source(_source) {}
   
   explicit message_set(void) {}
   
   ~message_set(void) {}
};

}

#ifdef COMPILE_MPI
BOOST_CLASS_TRACKING(process::message, boost::serialization::track_never)
BOOST_CLASS_TRACKING(process::message_set, boost::serialization::track_never)
#endif

#endif
