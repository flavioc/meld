
// do NOT include this file directly, please include runtime/objs.hpp

#include <iostream>

#ifndef RUNTIME_OBJS_HPP
#error "Please include runtime/objs.hpp instead"
#endif

struct array
{
   private:

      std::atomic<vm::ref_count> refs{0};
      size_t cap;
      size_t size;
      vm::tuple_field *elems;

      static inline void remove(array *c)
      {
         mem::allocator<vm::tuple_field>().deallocate(c->elems, c->cap);
         mem::allocator<array>().deallocate(c, 1);
      }

   public:

      inline size_t get_size() const { return size; }
      inline vm::tuple_field get_item(const size_t i) const { return elems[i]; }

      inline void set_index(const size_t i, const vm::tuple_field f, vm::type *type, vm::candidate_gc_nodes& gc_nodes)
      {
         vm::tuple_field old(get_item(i));
         elems[i] = f;
         decrement_runtime_data(old, type, gc_nodes);
         increment_runtime_data(f, type->get_type());
      }

      inline void inc_refs(void)
      {
         refs++;
      }

      inline void dec_refs(vm::type *type, vm::candidate_gc_nodes& gc_nodes)
      {
         assert(refs > 0);
         if(--refs == 0)
            destroy(type, gc_nodes);
      }

      inline void destroy(vm::type *type, vm::candidate_gc_nodes& gc_nodes)
      {
         if(type->is_reference()) {
            for(size_t i(0); i < size; ++i)
               decrement_runtime_data(elems[i], type, gc_nodes);
         }
         remove(this);
      }

      inline bool exists(const vm::tuple_field f, const vm::type *typ) const
      {
         const size_t size(lookup_type_size(typ->get_type()));
         for(size_t i(0); i < size; ++i) {
            if(memcmp(&f, &elems[i], size) == 0)
               return true;
         }
         return false;
      }

      inline void add(const vm::tuple_field f, vm::type *type)
      {
         if(size == cap) {
            vm::tuple_field *old(elems);

            elems = mem::allocator<vm::tuple_field>().allocate(cap * 2);
            memcpy(elems, old, sizeof(vm::tuple_field)*cap);
            mem::allocator<vm::tuple_field>().deallocate(elems, cap);
            cap *= 2;
         }

         elems[size++] = f;
         increment_runtime_data(f, type->get_type());
      }

      static inline array* create_empty(const size_t init_cap = 8, const size_t start_refs = 0)
      {
         array *a(mem::allocator<array>().allocate(1));
         a->refs = start_refs;
         a->cap = init_cap;
         a->size = 0;
         a->elems = mem::allocator<vm::tuple_field>().allocate(a->cap);
         return a;
      }

      static inline array* create_fill(vm::type *t, const size_t size, const vm::tuple_field f, const size_t start_refs = 0)
      {
         array *a(mem::allocator<array>().allocate(1));
         a->refs = start_refs;
         a->cap = size * 2;
         a->size = size;
         a->elems = mem::allocator<vm::tuple_field>().allocate(a->cap);
         for(size_t i(0); i < size; ++i)
            a->elems[i] = f;
         if(t->is_reference()) {
            for(size_t i(0); i < a->size; ++i)
               increment_runtime_data(a->elems[i], t->get_type());
         }
         return a;
      }

      static inline array* create_from_vector(vm::type *t, const std::vector<vm::tuple_field, mem::allocator<vm::tuple_field>>& v, const size_t start_refs = 0)
      {
         array *a(mem::allocator<array>().allocate(1));
         a->refs = start_refs;
         a->cap = v.size() * 2;
         a->size = v.size();
         a->elems = mem::allocator<vm::tuple_field>().allocate(a->cap);
         const vm::tuple_field* pv = &v[0];
         memcpy(a->elems, pv, v.size() * sizeof(vm::tuple_field));
         if(t->is_reference()) {
            for(size_t i(0); i < a->size; ++i)
               increment_runtime_data(a->elems[i], t->get_type());
         }
         return a;
      }

      static inline array* mutate(const array *old, vm::type *type, const size_t idx, const vm::tuple_field f, const size_t start_refs = 0)
      {
         array *a(mem::allocator<array>().allocate(1));
         a->refs = start_refs;
         a->cap = old->cap;
         a->size = old->size;
         a->elems = mem::allocator<vm::tuple_field>().allocate(old->cap);
         memcpy(a->elems, old->elems, sizeof(vm::tuple_field) * a->size);
         a->elems[idx] = f;
         if(type->is_reference()) {
            for(size_t i(0); i < a->size; ++i)
               increment_runtime_data(a->elems[i], type->get_type());
         }
         return a;
      }

      static inline array* mutate_add(const array *old, vm::type *type, const vm::tuple_field f, const size_t start_refs = 0)
      {
         array *a(mem::allocator<array>().allocate(1));
         a->refs = start_refs;
         if(old->size == old->cap)
            a->cap = 2 * old->cap;
         else
            a->cap = old->cap;
         a->size = old->size;
         a->elems = mem::allocator<vm::tuple_field>().allocate(a->cap);
         memcpy(a->elems, old->elems, sizeof(vm::tuple_field) * old->size);
         a->elems[a->size] = f;
         a->size++;
         if(type->is_reference()) {
            for(size_t i(0); i < a->size; ++i)
               increment_runtime_data(a->elems[i], type->get_type());
         }
         return a;
      }
};
