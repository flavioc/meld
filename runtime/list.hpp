#ifndef LIST_HPP
#define LIST_HPP

#include "conf.hpp"

#include <iostream>
#include <string>
#include <stack>
#include <list>

#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif

#include "utils/types.hpp"
#include "utils/atomic.hpp"
#include "utils/serialization.hpp"
#include "mem/base.hpp"
#include "vm/types.hpp"
#include "runtime/common.hpp"

#include "vm/defs.hpp"

namespace runtime
{

class cons: public mem::base
{
public:
   
   typedef cons* list_ptr;
   typedef list_ptr ptr;

   MEM_METHODS(cons)
   
private:
   
   utils::atomic<vm::ref_count> refs;
   list_ptr tail;
   const vm::tuple_field head;
   
   vm::list_type *type;
   
   inline void set_tail(list_ptr t)
   {
      tail = t;
      
      if(!is_null(tail))
         tail->inc_refs();
   }

public:
   
   const vm::tuple_field get_head(void) const { return head; }
   list_ptr get_tail(void) const { return tail; }
   vm::list_type* get_type(void) const { return type; }
   
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

   static inline bool has_refs(list_ptr ls)
   {
      if(is_null(ls))
         return true;
      return ls->refs > 0;
   }
   
   inline bool zero_refs(void) const { return refs == 0; }
   
   inline void destroy(void)
   {
      assert(zero_refs());
      if(!is_null(get_tail()))
         get_tail()->dec_refs();
      decrement_runtime_data(get_head(), type->get_subtype());
      delete this;
   }
   
   typedef void (*print_function)(std::ostream&, const vm::tuple_field&);

   void print(std::ostream& cout, const bool first, print_function print) const
   {
      if(first)
         cout << "[";
      else
         cout << ",";
      
      print(cout, head);
      
      if(is_null(tail))
         cout << "]";
      else
         tail->print(cout, false, print);
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
   
   static inline
	size_t size_list(const list_ptr ptr)
   {
      return sizeof(unsigned int) + sizeof(vm::tuple_field) * length(ptr);
   }
   
   static inline
	void pack(const list_ptr ptr, utils::byte *buf, const size_t buf_size, int *pos)
   {
      const size_t len(length(ptr));
      
      utils::pack<unsigned int>((void*)&len, 1, buf, buf_size, pos);
      
      list_ptr p(ptr);
      
      assert(*pos < (int)buf_size);
      
      for(size_t i(0); i < len; ++i) {
         if(is_null(p))
            return;
         
         utils::pack<vm::tuple_field>((void *)&(p->head), 1, buf, buf_size, pos);
         p = p->get_tail();
      }
   }
   
   static inline
	list_ptr unpack(utils::byte *buf, const size_t buf_size, int *pos, vm::list_type *t)
   {
      size_t len(0);
      
      utils::unpack<unsigned int>(buf, buf_size, pos, &len, 1);
      
      list_ptr prev(null_list());
      list_ptr init(null_list());
      
      while(len > 0) {
         vm::tuple_field head;

         utils::unpack<vm::tuple_field>(buf, buf_size, pos, &head, 1);
         
         list_ptr n(new cons(null_list(), head, t));
         
         if(is_null(prev))
            init = n;
         else
            prev->set_tail(n);
         
         prev = n;
         
         --len;
      }
      
      return init;
   }

   static inline
   list_ptr copy(list_ptr ptr)
   {
      if(is_null(ptr))
         return null_list();
         
      list_ptr init(new cons(null_list(), ptr->get_head(), ptr->type));
      list_ptr cur(init);
      
      ptr = ptr->get_tail();
   
      while (!is_null(ptr)) {
         cur->set_tail(new cons(null_list(), ptr->get_head(), ptr->type));
         cur = cur->get_tail();
         ptr = ptr->get_tail();
      }
      
      return init;
   }
   
   static inline
	void print(std::ostream& cout, list_ptr ls, print_function print)
	{
      if(is_null(ls))
         cout << "[]";
      else
         ls->print(cout, true, print);
   }

   typedef bool (*equal_function)(const vm::tuple_field&, const vm::tuple_field&);
   
   static inline
	bool equal(const list_ptr l1, const list_ptr l2, equal_function eq)
	{
      if(l1 == null_list() && l2 != null_list())
         return false;
      if(l1 != null_list() && l2 == null_list())
         return false;
      if(l1 == null_list() && l2 == null_list())
         return true;
         
      if(!eq(l1->get_head(), l2->get_head()))
         return false;
      
      return equal(l1->get_tail(), l2->get_tail(), eq);
   }
   
   static inline
	size_t length(const list_ptr ls)
	{
      if(is_null(ls))
         return 0;
         
      return 1 + length(ls->get_tail());
   }
   
   static inline
	vm::tuple_field get(const list_ptr ls, const size_t pos, const vm::tuple_field def)
	{
      if(is_null(ls))
         return def;
         
      if(pos == 0)
         return ls->get_head();
      
      return get(ls->get_tail(), pos - 1, def);
   }

	inline void stl_list(std::list<vm::tuple_field>& ls)
	{
		ls.push_back(get_head());

		if(!is_null(get_tail()))
			get_tail()->stl_list(ls);
	}

	static inline
	std::list<vm::tuple_field> stl_list(const list_ptr ls)
	{
		std::list<vm::tuple_field> ret;

		if(is_null(ls))
			return ret;

		ls->stl_list(ret);
		return ret;
	}

   explicit cons(list_ptr _tail, const vm::tuple_field _head, vm::list_type *_type):
      refs(0), head(_head), type(_type)
   {
      assert(type != NULL);
      increment_runtime_data(head, type->get_subtype());
      set_tail(_tail);
   }
};

typedef std::stack<vm::float_val, std::deque<vm::float_val, mem::allocator<vm::float_val> > > stack_float_list;
typedef std::stack<vm::int_val, std::deque<vm::int_val, mem::allocator<vm::int_val> > > stack_int_list;
typedef std::stack<vm::node_val, std::deque<vm::node_val, mem::allocator<vm::node_val> > > stack_node_list;
typedef std::stack<vm::tuple_field, std::deque<vm::tuple_field, mem::allocator<vm::tuple_field> > > stack_general_list;
typedef std::vector<vm::int_val, mem::allocator<vm::int_val> > vector_int_list;

static inline vm::tuple_field
build_from_int(const vm::int_val v)
{
   vm::tuple_field ret;
   ret.int_field = v;
   return ret;
}

static inline vm::tuple_field
build_from_float(const vm::float_val v)
{
   vm::tuple_field ret;
   ret.float_field = v;
   return ret;
}

static inline vm::tuple_field
build_from_node(const vm::node_val v)
{
   vm::tuple_field ret;
   ret.node_field = v;
   return ret;
}

static inline vm::tuple_field
build_from_field(const vm::tuple_field v)
{
   return v;
}

template <class TStack, class Convert>
static inline cons*
from_stack_to_list(TStack& stk, vm::tuple_field (*conv)(const Convert), vm::list_type *t)
{
   cons *ptr(cons::null_list());
   
   while(!stk.empty()) {
      ptr = new cons(ptr, conv(stk.top()), t);
      stk.pop();
   }
   
   return ptr;
}

static inline cons*
from_float_stack_to_list(stack_float_list& stk)
{
   return from_stack_to_list<stack_float_list>(stk, build_from_float, vm::TYPE_LIST_FLOAT);
}

static inline cons*
from_int_stack_to_list(stack_int_list& stk)
{
   return from_stack_to_list<stack_int_list>(stk, build_from_int, vm::TYPE_LIST_INT);
}

static inline cons*
from_node_stack_to_list(stack_node_list& stk)
{
   return from_stack_to_list<stack_node_list>(stk, build_from_node, vm::TYPE_LIST_NODE);
}

static inline cons*
from_general_stack_to_list(stack_general_list& stk, vm::list_type *t)
{
   return from_stack_to_list<stack_general_list>(stk, build_from_field, t);
}

template <class TStack, class Convert>
static inline cons*
from_stack_to_reverse_list(TStack& stk, vm::tuple_field (*conv)(const Convert), vm::list_type *t)
{
   TStack stk2;

   while(!stk.empty()) {
      stk2.push(stk.top());
      stk.pop();
   }

   return from_stack_to_list<TStack>(stk2, conv, t);
}

static inline cons*
from_int_stack_to_reverse_list(stack_int_list& stk)
{
   return from_stack_to_reverse_list<stack_int_list>(stk, build_from_int, vm::TYPE_LIST_INT);
}

static inline cons*
from_general_stack_to_reverse_list(stack_general_list& stk, vm::list_type *t)
{
   return from_stack_to_reverse_list<stack_general_list>(stk, build_from_field, t);
}

/* adds elements of the vector in reverse order */
template <class TVector, class Convert>
static inline cons*
from_vector_to_reverse_list(TVector& vec, vm::tuple_field (*conv)(const Convert), vm::list_type *t)
{
   cons *ptr(cons::null_list());

   for(typename TVector::iterator it(vec.begin()), end(vec.end()); it != end; ++it) {
      ptr = new cons(ptr, conv(*it), t);
   }

   return ptr;
}

static inline cons*
from_int_vector_to_reverse_list(vector_int_list& vec)
{
   return from_vector_to_reverse_list(vec, build_from_int, vm::TYPE_LIST_INT);
}

}

#endif
