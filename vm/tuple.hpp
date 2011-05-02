#ifndef VM_TUPLE_HPP
#define VM_TUPLE_HPP

#include "conf.hpp"

#include <ostream>
#include <list>

#ifdef COMPILE_MPI
#include <boost/serialization/serialization.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#endif

#include "vm/defs.hpp"
#include "vm/predicate.hpp"
#include "runtime/list.hpp"
#include "mem/allocator.hpp"
#include "mem/base.hpp"

namespace vm
{

typedef union {
	int_val int_field;
	float_val float_field;
	node_val node_field;
	ptr_val ptr_field;
} tuple_field;

class tuple: public mem::base<tuple>
{
private:

	predicate* pred;
   tuple_field *fields;
   
#ifdef COMPILE_MPI
   friend class boost::serialization::access;
   
   void save(boost::mpi::packed_oarchive&, const unsigned int) const;    
   void load(boost::mpi::packed_iarchive&, const unsigned int);
   
   BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

public:

   bool field_equal(const tuple&, const field_num) const;

   bool operator==(const tuple&) const;

#define define_set(NAME, TYPE, VAL) \
   inline void set_ ## NAME (const field_num& field, TYPE val) { VAL; }

   define_set(int, const int_val&, fields[field].int_field = val);
   define_set(float, const float_val&, fields[field].float_field = val);
   define_set(ptr, const ptr_val&, fields[field].ptr_field = val);
   define_set(bool, const bool_val&, set_int(field, val ? 1 : 0));
   define_set(node, const node_val&, fields[field].node_field = val);
   define_set(int_list, runtime::int_list*, set_ptr(field, (ptr_val)val); runtime::int_list::inc_refs(val));
   define_set(float_list, runtime::float_list*, set_ptr(field, (ptr_val)val); runtime::float_list::inc_refs(val));
   define_set(node_list, runtime::node_list*, set_ptr(field, (ptr_val)val); runtime::node_list::inc_refs(val));

   inline void set_nil(const field_num& field) { set_ptr(field, null_ptr_val); }
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
   define_get(ptr_val, ptr, fields[field].ptr_field);
   define_get(bool_val, bool, get_int(field) ? true : false);
   define_get(node_val, node, fields[field].node_field);
   define_get(runtime::int_list*, int_list, (runtime::int_list*)get_ptr(field));
   define_get(runtime::float_list*, float_list, (runtime::float_list*)get_ptr(field));
   define_get(runtime::node_list*, node_list, (runtime::node_list*)get_ptr(field));

#undef define_get

   inline const bool is_aggregate(void) const { return pred->is_aggregate(); }
   
   void print(std::ostream&) const;
   
   tuple *copy(void) const;
   
   explicit tuple(void); // only used for serialization!
	explicit tuple(const predicate* pred);
	
   ~tuple(void);
};

std::ostream& operator<<(std::ostream& cout, const tuple& pred);

typedef std::list<tuple*, mem::allocator<vm::tuple*> > tuple_list;

}

#ifdef COMPILE_MPI
BOOST_CLASS_TRACKING(vm::tuple, boost::serialization::track_never)
#endif

#endif
