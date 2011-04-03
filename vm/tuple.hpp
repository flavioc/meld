#ifndef VM_TUPLE_HPP
#define VM_TUPLE_HPP

#include <ostream>

#include "vm/defs.hpp"
#include "vm/predicate.hpp"
#include "runtime/list.hpp"

namespace vm
{

class tuple
{
private:

	typedef struct {
		int_val int_field;
		float_val float_field;
		addr_val addr_field;
	} tuple_field;

	const predicate* pred;
   tuple_field *fields;

public:
   
   bool operator==(const tuple&) const;
   
#define define_set(NAME, TYPE, VAL) \
   inline void set_ ## NAME (const field_num& field, TYPE val) { VAL; }
   
   define_set(int, const int_val&, fields[field].int_field = val);
   define_set(float, const float_val&, fields[field].float_field = val);
   define_set(addr, const addr_val&, fields[field].addr_field = val);
   define_set(bool, const bool_val&, set_int(field, val ? 1 : 0));
   define_set(int_list, runtime::int_list*, set_addr(field, val); runtime::int_list::inc_refs(val));
   define_set(float_list, runtime::float_list*, set_addr(field, val); runtime::float_list::inc_refs(val));
   define_set(addr_list, runtime::addr_list*, set_addr(field, val); runtime::addr_list::inc_refs(val));
   
   inline void set_nil(const field_num& field) { set_addr(field, NULL); }
#undef define_set
   
   size_t num_fields(void) const { return pred->num_fields(); }
   
   std::string pred_name(void) const { return pred->get_name(); }
   
   inline const predicate* get_predicate(void) const { return pred; }
   
   inline const predicate_id get_predicate_id(void) const { return pred->get_id(); }
   
   field_type get_field_type(const field_num& field) const { return pred->get_field_type(field); }
   
#define define_get(RET, NAME, VAL) \
   inline RET get_ ## NAME (const field_num& field) const { return VAL; }
   
   define_get(int_val, int, fields[field].int_field);
   define_get(float_val, float, fields[field].float_field);
   define_get(addr_val, addr, fields[field].addr_field);
   define_get(bool_val, bool, get_int(field) ? true : false);
   define_get(runtime::int_list*, int_list, (runtime::int_list*)get_addr(field));
   define_get(runtime::float_list*, float_list, (runtime::float_list*)get_addr(field));
   define_get(runtime::addr_list*, addr_list, (runtime::addr_list*)get_addr(field));
   
#undef define_get
   
   void print(std::ostream&) const;
   
	explicit tuple(const predicate* pred);
	
   ~tuple(void);
	
};

std::ostream& operator<<(std::ostream& cout, const tuple& pred);

}

#endif
