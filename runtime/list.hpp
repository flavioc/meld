#ifndef LIST_HPP
#define LIST_HPP

#include "conf.hpp"

#include <iostream>
#include <string>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#endif
#include "utils/types.hpp"
#include "mem/base.hpp"

#include "vm/defs.hpp"

namespace runtime
{

template <typename T>
class cons: public mem::base< cons<T> >
{
public:
   
   typedef cons* list_ptr;
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
   
   static inline size_t size_list(const list_ptr ptr, const size_t elem_size)
   {
      size_t ret(sizeof(utils::byte));
      
      if(is_null(ptr))
         return ret;
      else
         return ret + elem_size + size_list(ptr->get_tail(), elem_size);
   }
   
   static inline void pack(const list_ptr ptr, MPI_Datatype typ,
               utils::byte *buf, const size_t buf_size, int *pos, MPI_Comm comm)
   {
      utils::byte more;
      
      if(is_null(ptr)) {
         more = 0;
         MPI_Pack(&more, 1, MPI_UNSIGNED_CHAR, buf, buf_size, pos, comm);
      } else {
         more = 1;
         MPI_Pack(&more, 1, MPI_UNSIGNED_CHAR, buf, buf_size, pos, comm);
         MPI_Pack((void *)&(ptr->head), 1, typ, buf, buf_size, pos, comm);
         pack(ptr->get_tail(), typ, buf, buf_size, pos, comm);
      }
   }
   
   static inline list_ptr unpack(MPI_Datatype typ, utils::byte *buf,
               const size_t buf_size, int *pos, MPI_Comm comm)
   {
      utils::byte more;
      
      MPI_Unpack(buf, buf_size, pos, &more, 1, MPI_UNSIGNED_CHAR, comm);
      
      if(more == 0)
         return null_list();
      
      T head;
      
      MPI_Unpack(buf, buf_size, pos, &head, 1, typ, comm);
      
      return new cons(unpack(typ, buf, buf_size, pos, comm), head);
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