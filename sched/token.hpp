
#ifndef SCHED_TOKEN_HPP
#define SCHED_TOKEN_HPP

#include "conf.hpp"

#ifdef COMPILE_MPI
#include <boost/serialization/serialization.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>

namespace sched
{
   
class token
{
private:
   
   enum {
      WHITE,
      BLACK
   } type;
   
   int count;
   
   friend class boost::serialization::access;
   
   inline void save(boost::mpi::packed_oarchive& ar, const unsigned int) const
   {
      bool to_save = (type == WHITE);
      
      ar & to_save;
      
      ar & count;
   }

   inline void load(boost::mpi::packed_iarchive& ar, const unsigned int)
   {
      bool to_load;
      ar & to_load;
      
      type = (to_load ? WHITE : BLACK);
      ar & count;
   }

   BOOST_SERIALIZATION_SPLIT_MEMBER()
   
public:
   
   void transmitted(void) { ++count; }
   void transmitted(const int plus) { count += plus; }
   
   void add_count(const int plus) { count += plus; }
   
   inline const bool is_zero(void) const { return count == 0; }
   
   void received(void) { --count; }
   
   void reset_count(void) { count = 0; }
   inline const int get_count(void) const { return count; }
   
   void set_white(void) { type = WHITE; }
   void set_black(void) { type = BLACK; }
   
   inline const bool is_white(void) const { return type == WHITE; }
   inline const bool is_black(void) const { return type == BLACK; }
   
   explicit token(void): type(BLACK), count(0) {}
   
   ~token(void) {}
};

}
#endif

#endif
