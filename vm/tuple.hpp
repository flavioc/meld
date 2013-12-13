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

class tuple: public mem::base
{
private:

	predicate* pred;
   tuple_field *fields;

   void copy_field(tuple *, const field_num) const;

public:

   MEM_METHODS(tuple)

   bool field_equal(const tuple&, const field_num) const;

   bool operator==(const tuple&) const;

#define define_set(NAME, TYPE, VAL) \
   inline void set_ ## NAME (const field_num& field, TYPE val) { VAL; }

   define_set(bool, const bool_val&, SET_FIELD_BOOL(fields[field], val));
   define_set(int, const int_val&, SET_FIELD_INT(fields[field], val));
   define_set(float, const float_val&, SET_FIELD_FLOAT(fields[field], val));
   define_set(ptr, const ptr_val&, SET_FIELD_PTR(fields[field], val));
   define_set(node, const node_val&, SET_FIELD_NODE(fields[field], val));
	define_set(string, const runtime::rstring::ptr, SET_FIELD_STRING(fields[field], val); val->inc_refs());
   define_set(cons, runtime::cons*, SET_FIELD_CONS(fields[field], val); runtime::cons::inc_refs(val));
   define_set(struct, runtime::struct1*, SET_FIELD_STRUCT(fields[field], val); val->inc_refs());

   inline void set_nil(const field_num& field) { SET_FIELD_CONS(fields[field], runtime::cons::null_list()); }
   inline void set_field(const field_num& field, const tuple_field& f) { fields[field] = f; }
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

   tuple_field get_field(const field_num& field) const { return fields[field]; }
   
#define define_get(RET, NAME, VAL) \
   inline RET get_ ## NAME (const field_num& field) const { return VAL; }

   define_get(int_val, int, FIELD_INT(fields[field]));
   define_get(float_val, float, FIELD_FLOAT(fields[field]));
   define_get(ptr_val, ptr, FIELD_PTR(fields[field]));
   define_get(bool_val, bool, FIELD_BOOL(fields[field]));
   define_get(node_val, node, FIELD_NODE(fields[field]));
	define_get(runtime::rstring::ptr, string, FIELD_STRING(fields[field]));
   define_get(runtime::cons*, cons, FIELD_CONS(fields[field]));
   define_get(runtime::struct1*, struct, FIELD_STRUCT(fields[field]));

#undef define_get

	inline bool is_persistent(void) const { return pred->is_persistent_pred(); }
   inline bool is_aggregate(void) const { return pred->is_aggregate(); }
   inline bool is_linear(void) const { return pred->is_linear_pred(); }
   inline bool is_action(void) const { return pred->is_action_pred(); }
   inline bool is_reused(void) const { return pred->is_reused_pred(); }
   inline bool is_cycle(void) const { return pred->is_cycle_pred(); }
   
   void print(std::ostream&) const;
#ifdef USE_UI
	json_spirit::Value dump_json(void) const;
#endif
   
   tuple *copy_except(const field_num) const;
   tuple *copy(void) const;
   
   explicit tuple(void); // only used for serialization!
	explicit tuple(const predicate* pred);
	
   ~tuple(void);
};

std::ostream& operator<<(std::ostream& cout, const tuple& pred);

typedef std::list<tuple*, mem::allocator<vm::tuple*> > tuple_list;

}

#endif
