
// do NOT include this file directly, please include runtime/objs.hpp

#include <iostream>
#include <unordered_set>

#ifndef RUNTIME_OBJS_HPP
#error "Please include runtime/objs.hpp instead"
#endif

struct set
{
   private:

      std::atomic<vm::ref_count> refs{0};
      using hash_type = vm::ptr_val;
      static_assert(sizeof(hash_type) == sizeof(vm::tuple_field), "hash_type must be as long as vm::tuple_field.");
//      using data_type = std::unordered_set<hash_type, utils::fnv1_hasher<hash_type>,
 //        std::equal_to<hash_type>, mem::allocator<hash_type>>;
      using data_type = std::set<hash_type, std::less<hash_type>,
            mem::allocator<hash_type>>;
      data_type data;
      using iterator = data_type::iterator;
      using const_iterator = data_type::const_iterator;

      static inline void remove(set *s)
      {
         mem::allocator<set>().destroy(s);
         mem::allocator<set>().deallocate(s, 1);
      }

      static inline set* create()
      {
         set *s(mem::allocator<set>().allocate(1));
         mem::allocator<set>().construct(s);
         return s;
      }

   public:

      inline size_t get_size() const { return data.size(); }

      inline bool exists(const vm::tuple_field t) const
      {
         return data.find(t.ptr_field) != data.end();
      }

      inline void inc_refs(void)
      {
         refs++;
      }

      iterator begin() { return data.begin(); }
      iterator end() { return data.end(); }
      const_iterator begin() const { return data.begin(); }
      const_iterator end() const { return data.end(); }

      inline void dec_refs(vm::type *type, vm::candidate_gc_nodes& gc_nodes)
      {
         assert(refs > 0);
         if(--refs == 0)
            destroy(type, gc_nodes);
      }

      inline void destroy(vm::type *type, vm::candidate_gc_nodes& gc_nodes)
      {
         if(type->is_reference()) {
            for(auto p : data) {
               vm::tuple_field s;
               s.ptr_field = p;
               decrement_runtime_data(s, type, gc_nodes);
            }
         }
         remove(this);
      }

      inline void add(const vm::tuple_field f, vm::type *type)
      {
         data.insert(f.ptr_field);
         increment_runtime_data(f, type->get_type());
      }

      static inline set* create_empty(const size_t start_refs = 0)
      {
         set *a(create());
         a->refs = start_refs;
         return a;
      }

      static inline set* create_from_vector(vm::type *t, const std::vector<vm::tuple_field, mem::allocator<vm::tuple_field>>& v, const size_t start_refs = 0)
      {
         set *a(create());
         a->refs = start_refs;
         if(t->is_reference()) {
            for(const vm::tuple_field& f : v) 
               a->add(f, t);
         }
         return a;
      }

      static inline set* mutate_add(const set *old, vm::type *type, const vm::tuple_field f, const size_t start_refs = 0)
      {
         set *a(create());
         a->refs = start_refs;
         a->data = old->data;
         a->add(f, type);
         //std::cout << a->data.bucket_count() << "/" << a->data.max_bucket_count() << std::endl;
         //std::cout << a->data.size() << "/" << a->data.load_factor() << std::endl;
         return a;
      }
};

