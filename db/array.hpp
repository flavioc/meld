
#ifndef DB_ARRAY_HPP
#define DB_ARRAY_HPP

#include <vector>
#include <string>

#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "vm/defs.hpp"

namespace db
{

struct array {
   vm::tuple_field *data{nullptr};
   size_t num_tuples;

   struct iterator {
      vm::tuple_field *ptr{nullptr};
      size_t step{0};

      inline vm::tuple *operator*() {
         vm::tuple *tpl((vm::tuple*)(((utils::byte*)ptr)-sizeof(vm::tuple)));
         assert(tpl->getfp() == ptr);
         return tpl;
      }

      inline bool operator==(const iterator& other) const
      {
         return ptr == other.ptr;
      }

      inline bool operator!=(const iterator& other) const
      {
         return !operator==(other);
      }

      inline iterator& operator++()
      {
         ptr += step;
         return *this;
      }

      inline iterator operator++(int)
      {
         ptr += step;
         return *this;
      }

      explicit inline iterator(const size_t start, const size_t num_args, vm::tuple_field *data):
         ptr(data + num_args * start),
         step(num_args)
      {
      }
   };

   inline iterator begin(const vm::predicate *pred) { return iterator(0, pred->num_fields(), data); }
   inline iterator end(const vm::predicate *pred) { return iterator(num_tuples, pred->num_fields(), data); }
   inline iterator begin(const vm::predicate *pred) const { return iterator(0, pred->num_fields(), data); }
   inline iterator end(const vm::predicate *pred) const { return iterator(num_tuples, pred->num_fields(), data); }

   inline void init(const size_t _size, const vm::predicate *pred) {
      const size_t total(pred->num_fields() * _size);
      data = mem::allocator<vm::tuple_field>().allocate(total);
      num_tuples = _size;
   }

   inline vm::tuple *add_next(const vm::predicate *pred, const size_t i)
   {
      vm::tuple *tpl((vm::tuple*)(((utils::byte*)(data + i * pred->num_fields())) - sizeof(vm::tuple)));
      assert(tpl->getfp() == data + i * pred->num_fields());
      return tpl;
   }

   inline size_t size() const { return num_tuples; }

   std::vector<std::string> get_print_strings(const vm::predicate *pred) const
   {
      std::vector<std::string> vec;
      for(auto it(begin(pred)), e(end(pred)); it != e; ++it) {
         vec.push_back((*it)->to_str(pred));
      }
      return vec;
   }

   explicit array() {}

   inline void wipeout(const vm::predicate *pred, vm::candidate_gc_nodes& gc_nodes) {
      for(auto it(begin(pred)), e(end(pred)); it != e; ++it) {
         vm::tuple *tpl(*it);
         tpl->destructor(pred, gc_nodes);
      }
      mem::allocator<vm::tuple_field>().deallocate(data, num_tuples * pred->num_fields());
   }
};

}

#endif
