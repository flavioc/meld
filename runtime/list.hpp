#ifndef LIST_HPP
#define LIST_HPP

#include <iostream>

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
typedef cons<vm::addr_val> addr_list;

}

#endif