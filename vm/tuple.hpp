#ifndef VM_TUPLE_HPP
#define VM_TUPLE_HPP

#include "conf.hpp"

#include <ostream>
#include <list>

#ifdef COMPILE_MPI
#include <mpi.h>
#endif

#ifdef USE_UI
#include <json_spirit.h>
#endif

#include "vm/defs.hpp"
#include "vm/predicate.hpp"
#include "runtime/objs.hpp"
#include "mem/allocator.hpp"
#include "mem/base.hpp"
#include "utils/types.hpp"

namespace vm
{

class tuple
{
private:

	bool to_delete;
	predicate* pred;
   tuple_field *fields;

   void copy_field(tuple *, const field_num) const;

   inline tuple_field *getfp(void) { return (tuple_field*)(this + 1); }
   inline const tuple_field *getfp(void) const { return (tuple_field*)(this + 1); }

public:

   MEM_METHODS(tuple)

   bool field_equal(const tuple&, const field_num) const;

   bool operator==(const tuple&) const;

#define define_set(NAME, TYPE, VAL) \
   inline void set_ ## NAME (const field_num& field, TYPE val) { VAL; }

   define_set(bool, const bool_val&, SET_FIELD_BOOL(getfp()[field], val));
   define_set(int, const int_val&, SET_FIELD_INT(getfp()[field], val));
   define_set(float, const float_val&, SET_FIELD_FLOAT(getfp()[field], val));
   define_set(ptr, const ptr_val&, SET_FIELD_PTR(getfp()[field], val));
   define_set(node, const node_val&, SET_FIELD_NODE(getfp()[field], val));
	define_set(string, const runtime::rstring::ptr, SET_FIELD_STRING(getfp()[field], val); val->inc_refs());
   define_set(cons, runtime::cons*, SET_FIELD_CONS(getfp()[field], val); runtime::cons::inc_refs(val));
   define_set(struct, runtime::struct1*, SET_FIELD_STRUCT(getfp()[field], val); val->inc_refs());

   inline void set_nil(const field_num& field) { SET_FIELD_CONS(getfp()[field], runtime::cons::null_list()); }
   inline void set_field(const field_num& field, const tuple_field& f) { getfp()[field] = f; }
#undef define_set

   size_t num_fields(void) const { return pred->num_fields(); }

   std::string pred_name(void) const { return pred->get_name(); }

   inline const predicate* get_predicate(void) const { return pred; }

   inline predicate_id get_predicate_id(void) const { return pred->get_id(); }
   
   size_t get_storage_size(void) const;
   
   void pack(utils::byte *, const size_t, int *) const;
   void load(utils::byte *, const size_t, int *);

   void copy_runtime(void);
   
   static tuple* unpack(utils::byte *, const size_t, int *, vm::program *);

   type* get_field_type(const field_num& field) const { return pred->get_field_type(field); }

   tuple_field get_field(const field_num& field) const { return getfp()[field]; }
   
#define define_get(RET, NAME, VAL) \
   inline RET get_ ## NAME (const field_num& field) const { return VAL; }

   define_get(int_val, int, FIELD_INT(getfp()[field]));
   define_get(float_val, float, FIELD_FLOAT(getfp()[field]));
   define_get(ptr_val, ptr, FIELD_PTR(getfp()[field]));
   define_get(bool_val, bool, FIELD_BOOL(getfp()[field]));
   define_get(node_val, node, FIELD_NODE(getfp()[field]));
	define_get(runtime::rstring::ptr, string, FIELD_STRING(getfp()[field]));
   define_get(runtime::cons*, cons, FIELD_CONS(getfp()[field]));
   define_get(runtime::struct1*, struct, FIELD_STRUCT(getfp()[field]));

#undef define_get

	inline bool is_persistent(void) const { return pred->is_persistent_pred(); }
   inline bool is_aggregate(void) const { return pred->is_aggregate(); }
   inline bool is_linear(void) const { return pred->is_linear_pred(); }
   inline bool is_action(void) const { return pred->is_action_pred(); }
   inline bool is_reused(void) const { return pred->is_reused_pred(); }
   inline bool is_cycle(void) const { return pred->is_cycle_pred(); }
   inline bool is_reused_or_pred(void) const { return is_reused() || is_persistent(); }
   
   void print(std::ostream&) const;
#ifdef USE_UI
	json_spirit::Value dump_json(void) const;
#endif
   
   tuple *copy_except(const field_num) const;
   tuple *copy(void) const;

   inline bool must_be_deleted(void) const { return to_delete; }
   inline void will_delete(void) { to_delete = true; }
   inline void will_not_delete(void) { to_delete = false; }
   inline bool can_be_consumed(void) const { return !to_delete; }

   static tuple* create(const predicate* pred) {
      const size_t size(sizeof(vm::tuple) + sizeof(tuple_field) * pred->num_fields());
      vm::tuple *ptr((vm::tuple*)mem::center::allocate(size, 1));
      new (ptr) vm::tuple(pred);
      return ptr;
   }

   static void destroy(tuple *tpl) {
      const size_t size(sizeof(vm::tuple) + sizeof(tuple_field) * tpl->num_fields());
      mem::center::deallocate(tpl, size, 1);
      tpl->~tuple();
   }
   
private:

	explicit tuple(const predicate* pred);
	
   ~tuple(void);
};

std::ostream& operator<<(std::ostream& cout, const tuple& pred);

typedef std::list<tuple*, mem::allocator<vm::tuple*> > tuple_list;

}

#endif
