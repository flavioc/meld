#ifndef LIST_HPP
#define LIST_HPP

#include "conf.hpp"

#include <iostream>
#include <string>
#include <stack>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif
#include "utils/types.hpp"
#include "utils/atomic.hpp"
#include "mem/base.hpp"

#include "vm/defs.hpp"

namespace runtime
{

template <typename T>
class cons: public mem::base
{
public:
   
   typedef cons* list_ptr;
   typedef list_ptr ptr;

   MEM_METHODS(cons<T>)
   
private:
   
   list_ptr tail;
   const T head;
   
   utils::atomic<size_t> refs;
   
   inline void set_tail(list_ptr t)
   {
      tail = t;
      
      if(!is_null(tail))
         tail->inc_refs();
   }

public:
   
   const T get_head(void) const { return head; }
   list_ptr get_tail(void) const { return tail; }
   
   inline void inc_refs(void)
	 {
      refs++;
   }
   
   inline void dec_refs(void)
	 {
      assert(refs > 0);
      refs--;
      if(zero_refs())
         destroy();
   }
   
   inline bool zero_refs(void) const { return refs == 0; }
   
   inline void destroy(void)
   {
      assert(zero_refs());
      if(!is_null(get_tail()))
         get_tail()->dec_refs();
      delete this;
   }
   
   void print(std::ostream& cout, const bool first) const
   {
      if(first)
         cout << "[";
      else
         cout << ",";
      
      cout << head;
      
      if(is_null(tail))
         cout << "]";
      else
         tail->print(cout, false);
   }
   
   static inline list_ptr null_list(void) { return (list_ptr)0; }
   
   static inline bool is_null(const list_ptr ls) { return ls == null_list(); }
   
   static inline void dec_refs(list_ptr ls)
	 {
      if(!is_null(ls))
         ls->dec_refs();
   }
   
   static inline void inc_refs(list_ptr ls)
	 {
      if(!is_null(ls))
         ls->inc_refs();
   }
   
#ifdef COMPILE_MPI
   static inline
	size_t size_list(const list_ptr ptr, const size_t elem_size)
   {
      return sizeof(unsigned int) + elem_size * length(ptr);
   }
   
   static inline
	void pack(const list_ptr ptr, MPI_Datatype typ,
          utils::byte *buf, const size_t buf_size, int *pos, MPI_Comm comm)
   {
      const size_t len(length(ptr));
      
      MPI_Pack((void*)&len, 1, MPI_UNSIGNED, buf, buf_size, pos, comm);
      
      list_ptr p(ptr);
      
      assert(*pos < (int)buf_size);
      
      for(size_t i(0); i < len; ++i) {
         if(is_null(p))
            return;
         
         MPI_Pack((void *)&(p->head), 1, typ, buf, buf_size, pos, comm);
         p = p->get_tail();
      }
   }
   
   static inline
	list_ptr unpack(MPI_Datatype typ, utils::byte *buf,
            const size_t buf_size, int *pos, MPI_Comm comm)
   {
      size_t len(0);
      
      MPI_Unpack(buf, buf_size, pos, &len, 1, MPI_UNSIGNED, comm);
      
      list_ptr prev(null_list());
      list_ptr init(null_list());
      
      while(len > 0) {
         T head;

         MPI_Unpack(buf, buf_size, pos, &head, 1, typ, comm);
         
         list_ptr n(new cons(null_list(), head));
         
         if(is_null(prev))
            init = n;
         else
            prev->set_tail(n);
         
         prev = n;
         
         --len;
      }
      
      return init;
   }
#endif

   static inline
   list_ptr copy(list_ptr ptr)
   {
      if(is_null(ptr))
         return null_list();
         
      list_ptr init(new cons(null_list(), ptr->get_head()));
      list_ptr cur(init);
      
      ptr = ptr->get_tail();
   
      while (!is_null(ptr)) {
         cur->set_tail(new cons(null_list(), ptr->get_head()));
         cur = cur->get_tail();
         ptr = ptr->get_tail();
      }
      
      return init;
   }
   
   static inline
	 void print(std::ostream& cout, list_ptr ls)
	 {
      if(is_null(ls))
         cout << "[]";
      else
         cout << *ls;
   }
   
   static inline
	 bool equal(const list_ptr l1, const list_ptr l2)
	 {
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
   
   static inline
	size_t length(const list_ptr ls)
	{
      if(is_null(ls))
         return 0;
         
      return 1 + length(ls->get_tail());
   }
   
   static inline
	T get(const list_ptr ls, const size_t pos, const T def)
	{
      if(is_null(ls))
         return def;
         
      if(pos == 0)
         return ls->get_head();
      
      return get(ls->get_tail(), pos - 1, def);
   }
   
   explicit cons(list_ptr _tail, const T _head):
      head(_head), refs(0)
   {
      set_tail(_tail);
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
typedef std::stack<vm::float_val, std::deque<vm::float_val, mem::allocator<vm::float_val> > > stack_float_list;

static inline float_list*
from_stack_to_list(stack_float_list& stk)
{
   float_list *ptr(float_list::null_list());
   
   while(!stk.empty()) {
      ptr = new float_list(ptr, stk.top());
      stk.pop();
   }
   
   return ptr;
}

}

#endif
