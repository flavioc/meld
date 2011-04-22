#ifndef LIST_HPP
#define LIST_HPP

#include "conf.hpp"

#include <iostream>
#ifdef COMPILE_MPI
#include <boost/serialization/serialization.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#endif

#include "vm/defs.hpp"

namespace runtime
{

template <typename T>
class cons
{
public:
   
   typedef cons<T>* list_ptr;
   typedef list_ptr ptr;
   
private:
   
   list_ptr tail;
   const T head;
   
   typedef unsigned int ref_count;
   
   ref_count refs;
   
#ifdef COMPILE_MPI
   friend class boost::serialization::access;
   
   enum list_serialize { END_LIST = 0, ANOTHER_LIST = 1 };
   
   void save(boost::mpi::packed_oarchive& ar, const unsigned int version) const
   {
      (void)version;
      
      bool more = true;
      
      ar & more;
      ar & head;
      
      if(is_null(tail)) {
         more = false;
         ar & more;
      } else
         tail->save(ar, version);
   }
   
   void load(boost::mpi::packed_iarchive& ar, const unsigned int version)
   {  
   }
   
   BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

public:
   
   const T get_head(void) const { return head; }
   const list_ptr get_tail(void) const { return tail; }
   
   inline void inc_refs(void) {
      ++refs;
   }
   
   inline void dec_refs(void) {
      --refs;
      if(refs == 0) {
         if(!is_null(get_tail()))
            get_tail()->dec_refs();
         delete this;
      }
   }
   
   void print(std::ostream& cout, const bool first) const
   {
      if(first)
         cout << "[";
      else
         cout << ",";
      
      cout << head << "(" << refs << ")";
      
      if(is_null(tail))
         cout << "]";
      else
         tail->print(cout, false);
   }
   
   static inline list_ptr null_list(void) { return (list_ptr)0; }
   
   static inline bool is_null(const list_ptr ls) { return ls == null_list(); }
   
   static inline void dec_refs(list_ptr ls) {
      if(!is_null(ls))
         ls->dec_refs();
   }
   
   static inline void inc_refs(list_ptr ls) {
      if(!is_null(ls))
         ls->inc_refs();
   }
   
#ifdef COMPILE_MPI
   static void save_list(boost::mpi::packed_oarchive& ar, const list_ptr ptr)
   {
      if(is_null(ptr)) {
         bool more = false;
         ar & more;
      } else
         ptr->save(ar, 0);
   }
   
   static list_ptr load_list(boost::mpi::packed_iarchive& ar, const bool first = true)
   {  
      bool more;
      
      ar & more;
      
      if(!more)
         return null_list();
      
      T head;
      
      ar & head;
      
      return new cons(load_list(ar, false), head); 
   }
#endif
   
   static inline void print(std::ostream& cout, list_ptr ls) {
      if(is_null(ls))
         cout << "[]";
      else
         cout << *ls;
   }
   
   static inline bool equal(const list_ptr l1, const list_ptr l2) {
      if(l1 == null_list() && l2 != null_list())
         return false;
      if(l1 != null_list() && l2 == null_list())
         return false;
      if(l1 == null_list() && l2 == null_list())
         return true;
         
      if(l1->get_head() != l2->get_head())
         return false;
      
      return equal(l1->get_tail(), l2->get_tail());
   }
   
   explicit cons(list_ptr _tail, const T _head):
      tail(_tail), head(_head), refs(0)
   {
      if(!is_null(tail))
         tail->inc_refs();
   }
};

template <typename T>
std::ostream&
operator<<(std::ostream& cout, const cons<T>& ls)
{
   ls.print(cout, true);
   
   return cout;
}

typedef cons<vm::int_val> int_list;
typedef cons<vm::float_val> float_list;
typedef cons<vm::node_val> node_list;

}

#endif