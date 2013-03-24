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
#include "runtime/list.hpp"
#include "runtime/string.hpp"
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

   define_set(int, const int_val&, fields[field].int_field = val);
   define_set(float, const float_val&, fields[field].float_field = val);
   define_set(ptr, const ptr_val&, fields[field].ptr_field = val);
   define_set(bool, const bool_val&, set_int(field, val ? 1 : 0));
   define_set(node, const node_val&, fields[field].node_field = val);
	define_set(string, const runtime::rstring::ptr, set_ptr(field, (ptr_val)val); val->inc_refs());
   define_set(int_list, runtime::int_list*, set_ptr(field, (ptr_val)val); runtime::int_list::inc_refs(val));
   define_set(float_list, runtime::float_list*, set_ptr(field, (ptr_val)val); runtime::float_list::inc_refs(val));
   define_set(node_list, runtime::node_list*, set_ptr(field, (ptr_val)val); runtime::node_list::inc_refs(val));

   inline void set_nil(const field_num& field) { set_ptr(field, null_ptr_val); }
   inline void set_field(const field_num& field, tuple_field& f) { fields[field] = f; }
#undef define_set

   size_t num_fields(void) const { return pred->num_fields(); }

   std::string pred_name(void) const { return pred->get_name(); }

   inline const predicate* get_predicate(void) const { return pred; }

   inline predicate_id get_predicate_id(void) const { return pred->get_id(); }
   
   size_t get_storage_size(void) const;
   
   void pack(utils::byte *, const size_t, int *) const;
   void load(utils::byte *, const size_t, int *);
   
   static tuple* unpack(utils::byte *, const size_t, int *, vm::program *);

   field_type get_field_type(const field_num& field) const { return pred->get_field_type(field); }

   tuple_field get_field(const field_num& field) const { return fields[field]; }
   
#define define_get(RET, NAME, VAL) \
   inline RET get_ ## NAME (const field_num& field) const { return VAL; }

   define_get(int_val, int, fields[field].int_field);
   define_get(float_val, float, fields[field].float_field);
   define_get(ptr_val, ptr, fields[field].ptr_field);
   define_get(bool_val, bool, get_int(field) ? true : false);
   define_get(node_val, node, fields[field].node_field);
	define_get(runtime::rstring::ptr, string, (runtime::rstring::ptr)get_ptr(field));
   define_get(runtime::int_list*, int_list, (runtime::int_list*)get_ptr(field));
   define_get(runtime::float_list*, float_list, (runtime::float_list*)get_ptr(field));
   define_get(runtime::node_list*, node_list, (runtime::node_list*)get_ptr(field));

#undef define_get

	inline bool is_persistent(void) const { return pred->is_persistent_pred(); }
   inline bool is_aggregate(void) const { return pred->is_aggregate(); }
   inline bool is_linear(void) const { return pred->is_linear_pred(); }
   inline bool is_action(void) const { return pred->is_action_pred(); }
   inline bool is_reused(void) const { return pred->is_reused_pred(); }
   
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
