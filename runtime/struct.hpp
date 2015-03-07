
// do NOT include this file directly, please include runtime/objs.hpp

#ifndef RUNTIME_OBJS_HPP
#error "Please include runtime/objs.hpp instead"
#endif

struct struct1
{
   private:

      std::atomic<vm::ref_count> refs;
      vm::struct_type *typ;

      inline vm::tuple_field *get_fields(void) { return (vm::tuple_field*)(this + 1); }
      inline vm::tuple_field *get_fields(void) const { return (vm::tuple_field*)(this + 1);}

   public:

      inline vm::struct_type *get_type(void) const { return typ; }

      inline void inc_refs(void)
      {
         refs++;
      }

      inline size_t get_size(void) const { return typ->get_size(); }
      
      inline void dec_refs(vm::candidate_gc_nodes& gc_nodes)
      {
         assert(refs > 0);
         if(--refs == 0)
            destroy(gc_nodes);
      }

      inline void destroy(vm::candidate_gc_nodes& gc_nodes)
      {
         if(!typ->simple_composed_type()) {
            for(size_t i(0); i < get_size(); ++i)
               decrement_runtime_data(get_data(i), typ->get_type(i), gc_nodes);
         }
         remove(this);
      }

      inline void set_data(const size_t i, const vm::tuple_field& data)
      {
         *get_ptr(i) = data;
         increment_runtime_data(get_data(i), typ->get_type(i)->get_type());
      }

      inline vm::tuple_field get_data(const size_t i) const
      {
         return get_fields()[i];
      }

      inline vm::tuple_field* get_ptr(const size_t i)
      {
         return get_fields() + i;
      }

      static inline struct1* create(vm::struct_type *_typ) {
         const size_t size(sizeof(struct1) + sizeof(vm::tuple_field) * _typ->get_size());
         struct1 *p((struct1*)mem::allocator<utils::byte>().allocate(size));
         mem::allocator<struct1>().construct(p);
         p->typ = _typ;
         return p;
      }

      static inline void remove(struct1 *p) {
         const size_t size(sizeof(struct1) + sizeof(vm::tuple_field) * p->get_size());
         mem::allocator<utils::byte>().deallocate((utils::byte*)p, size);
      }

      struct1(void): refs(0) {}
};

