#ifndef VM_TUPLE_HPP
#define VM_TUPLE_HPP

#include <ostream>
#include <unordered_set>

#include "vm/defs.hpp"
#include "vm/predicate.hpp"
#include "runtime/objs.hpp"
#include "mem/allocator.hpp"
#include "utils/types.hpp"
#include "utils/mutex.hpp"
#include "utils/intrusive_list.hpp"

namespace vm
{

struct tuple
{
public:
   DECLARE_LIST_INTRUSIVE(tuple);

private:
#ifndef COMPILED
   utils::byte flags;
#endif

   void copy_field(type *, tuple *, const field_num) const;

   inline tuple_field *getfp(void) { return (tuple_field*)(this + 1); }
   inline const tuple_field *getfp(void) const { return (tuple_field*)(this + 1); }

public:

   bool field_equal(type *, const tuple&, const field_num) const;

   bool equal(const tuple&, vm::predicate *) const;

#define define_set(NAME, TYPE, VAL) \
   inline void set_ ## NAME (const field_num& field, TYPE val) { VAL; }

   void set_node(const field_num& field, const node_val& val);
   define_set(node_base, const vm::node_val&, SET_FIELD_NODE(getfp()[field], val));
   define_set(bool, const bool_val&, SET_FIELD_BOOL(getfp()[field], val));
   define_set(int, const int_val&, SET_FIELD_INT(getfp()[field], val));
   define_set(float, const float_val&, SET_FIELD_FLOAT(getfp()[field], val));
   define_set(ptr, const ptr_val&, SET_FIELD_PTR(getfp()[field], val));
	define_set(string, const runtime::rstring::ptr, SET_FIELD_STRING(getfp()[field], val); val->inc_refs());
   define_set(cons, runtime::cons*, SET_FIELD_CONS(getfp()[field], val); runtime::cons::inc_refs(val));
   define_set(struct, runtime::struct1*, SET_FIELD_STRUCT(getfp()[field], val); val->inc_refs());
   define_set(array, runtime::array*, SET_FIELD_ARRAY(getfp()[field], val); val->inc_refs());

   inline void set_nil(const field_num& field) { SET_FIELD_CONS(getfp()[field], runtime::cons::null_list()); }
   inline void set_field(const field_num& field, const tuple_field& f) { getfp()[field] = f; }
   void set_field_ref(const field_num&, const tuple_field&, const predicate*,
         candidate_gc_nodes& gc_nodes);
#undef define_set

   size_t get_storage_size(vm::predicate *) const;
   
   void pack(vm::predicate *, utils::byte *, const size_t, int *) const;
   void load(vm::predicate *, utils::byte *, const size_t, int *);

   void copy_runtime(const vm::predicate*);
   
   static tuple* unpack(utils::byte *, const size_t, int *, vm::program *);

   inline tuple_field get_field(const field_num& field) const { return getfp()[field]; }
   
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
   define_get(runtime::array*, array, FIELD_ARRAY(getfp()[field]));

#undef define_get

   std::string to_str(const vm::predicate *) const;
   void print(std::ostream&, const vm::predicate*) const;
   
   tuple *copy_except(vm::predicate *, const field_num) const;
   tuple *copy(vm::predicate *) const;

#ifndef COMPILED
#define TUPLE_DELETE_FLAG 0x01
#define TUPLE_UPDATED_FLAG 0x02
   inline bool must_be_deleted(void) const { return flags & TUPLE_DELETE_FLAG; }
   inline void will_delete(void) { flags |= TUPLE_DELETE_FLAG; }
   inline void will_not_delete(void) { flags &= ~TUPLE_DELETE_FLAG; }
   inline void set_updated(void) { flags |= TUPLE_UPDATED_FLAG; }
   inline void set_not_updated(void) { flags &= ~TUPLE_UPDATED_FLAG; }
   inline bool is_updated(void) const { return flags & TUPLE_UPDATED_FLAG; }
#endif

   inline static tuple* create(const predicate* pred) {
      const size_t size(sizeof(vm::tuple) + sizeof(tuple_field) * pred->num_fields());
      LOG_NEW_FACT();
      vm::tuple *ptr((vm::tuple*)mem::center::allocate(size, 1));
      ptr->init(pred);
      return ptr;
   }

   inline static void destroy(tuple *tpl, vm::predicate *pred, candidate_gc_nodes& gc_nodes)
   {
      const size_t size(sizeof(vm::tuple) + sizeof(tuple_field) * pred->num_fields());
      tpl->destructor(pred, gc_nodes);
      mem::allocator<utils::byte>().deallocate((utils::byte*)tpl, size);
   }
   
private:

   inline void init(const predicate *pred)
   {
#ifndef COMPILED
      flags = 0x00;
#endif
      assert(pred != nullptr);
#ifndef COMPILED
      memset(getfp(), 0, sizeof(tuple_field) * pred->num_fields());
#else
      (void)pred;
#endif
   }

   void destructor(vm::predicate*, candidate_gc_nodes&);
};

typedef utils::intrusive_list<vm::tuple> tuple_list;

}

#endif
