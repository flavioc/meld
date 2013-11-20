
#ifndef RUNTIME_STRUCT_HPP
#define RUNTIME_STRUCT_HPP

#include "conf.hpp"
#include "utils/types.hpp"
#include "utils/atomic.hpp"
#include "vm/types.hpp"
#include "mem/base.hpp"

namespace runtime
{

class struct1: public mem::base
{
   private:

      vm::tuple_field *fields;
      utils::atomic<vm::ref_count> refs;
      vm::struct_type *typ;

   public:

      inline vm::struct_type *get_type(void) const { return typ; }

      inline bool zero_refs(void) const { return refs == 0; }

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

      inline void destroy(void)
      {
         assert(zero_refs());
         for(size_t i(0); i < typ->get_size(); ++i) {
            decrement_runtime_data(fields[i], typ->get_type(i));
         }
         delete this;
      }

      inline void set_data(const size_t i, const vm::tuple_field& data)
      {
         fields[i] = data;
         increment_runtime_data(fields[i], typ->get_type(i));
      }

      inline vm::tuple_field get_data(const size_t i) const
      {
         return fields[i];
      }

      explicit struct1(vm::struct_type *_typ): refs(0), typ(_typ) {
         assert(typ);
         fields = mem::allocator<vm::tuple_field>().allocate(typ->get_size());
      }

      ~struct1(void) {
         mem::allocator<vm::tuple_field>().deallocate(fields, typ->get_size());
      }
};

}

#endif
