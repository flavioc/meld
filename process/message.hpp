
#ifndef PROCESS_MESSAGE_HPP
#define PROCESS_MESSAGE_HPP

#include <boost/serialization/serialization.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <ostream>

#include "db/node.hpp"
#include "db/tuple.hpp"

namespace process
{

class message
{
private:
   
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
   
public:
   
   db::node::node_id id;
   db::simple_tuple *data;
   
   explicit message(const db::node::node_id _id,
         const db::simple_tuple *_data):
      id(_id), data((db::simple_tuple*)_data)
   {
   }
   
   explicit message(void) {} // for serialization purposes
};


std::ostream& operator<<(std::ostream& cout, const message& msg);

}

#endif
