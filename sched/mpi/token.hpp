
#ifndef SCHED_MPI_TOKEN_HPP
#define SCHED_MPI_TOKEN_HPP

#include "conf.hpp"

#ifdef COMPILE_MPI
#include <boost/serialization/serialization.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi/datatype.hpp>

namespace sched
{
   
class token
{
private:
   
   static const bool WHITE = true;
   static const bool BLACK = false;
   
   bool type;
   int count;
   
   friend class boost::serialization::access;
   
   template <class Archive>
   inline void save(Archive& ar, const unsigned int) const
   {
      ar & type;
      ar & count;
   }

   template <class Archive>
   inline void load(Archive& ar, const unsigned int)
   {
      ar & type;
      ar & count;
   }

   BOOST_SERIALIZATION_SPLIT_MEMBER()

public:
   
   void transmitted(const int plus) { count += plus; }
   void received(const size_t total) { count -= total; }
   
   void add_count(const int plus) { count += plus; }
   
   inline bool is_zero(void) const { return count == 0; }
   
   void reset_count(void) { count = 0; }
   inline int get_count(void) const { return count; }
   
   void set_white(void) { type = WHITE; }
   void set_black(void) { type = BLACK; }
   
   inline bool is_white(void) const { return type == WHITE; }
   inline bool is_black(void) const { return type == BLACK; }
   
   explicit token(void): type(BLACK), count(0) {}
   
   ~token(void) {}
};

}

namespace boost { namespace mpi {
      template<> struct is_mpi_datatype<sched::token>
        : public mpl::true_ { };
    } }
    
BOOST_CLASS_IMPLEMENTATION(sched::token, boost::serialization::object_serializable)
BOOST_IS_BITWISE_SERIALIZABLE(sched::token)

#endif

#endif
