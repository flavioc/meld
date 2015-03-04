
#ifndef VM_STACK_HPP
#define VM_STACK_HPP

#include "vm/defs.hpp"
#include "mem/allocator.hpp"
#include "vm/instr.hpp"

namespace vm {

class call_stack {
   private:
   tuple_field *vec;
   size_t capacity;
   size_t size;

   inline void expand(void) {
      tuple_field *new_vec =
          mem::allocator<tuple_field>().allocate(capacity * 2);
      memcpy(new_vec + capacity, vec, sizeof(tuple_field) * capacity);
      mem::allocator<tuple_field>().deallocate(vec, capacity);
      vec = new_vec;
      capacity *= 2;
   }

   public:
   using iterator = tuple_field *;

   inline bool empty(void) const { return size == 0; }

   inline iterator begin(void) { return vec + capacity - size; }

   inline iterator end(void) { return vec + capacity; }

   inline void push(const size_t howmany = 1) {
      size += howmany;
      if (size > capacity) expand();
   }

   inline void pop(const size_t howmany = 1) { size -= howmany; }

   inline void push_regs(tuple_field *regs) {
      push(NUM_REGS);
      memcpy(get_stack_at(0), regs, NUM_REGS * sizeof(tuple_field));
   }

   inline void pop_regs(tuple_field *regs) {
      memcpy(regs, get_stack_at(0), NUM_REGS * sizeof(tuple_field));
      pop(NUM_REGS);
   }

   inline tuple_field *get_stack_at(const offset_num off) {
      return vec + capacity - size + off;
   }

   explicit call_stack(void)
       : vec(mem::allocator<tuple_field>().allocate(2 * NUM_REGS)),
         capacity(2 * NUM_REGS),
         size(0) {}

   ~call_stack(void) {
      mem::allocator<tuple_field>().deallocate(vec, capacity);
   }
};
}

#endif
