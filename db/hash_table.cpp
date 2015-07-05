
#include <iostream>

#include "db/hash_table.hpp"
#include "db/node.hpp"

using namespace vm;
using namespace std;

namespace db
{

size_t
hash_table::insert_front(vm::tuple *item, const vm::predicate *pred)
{
   abort();
   const uint_val id(hash_tuple(item, pred));
   tuple_list *bucket(table[mod_hash(id)]);
   bucket->push_front(item);
   elems++;
   return bucket->get_size();
}

void
hash_table::change_table(const std::uint16_t new_size_table, mem::node_allocator* alloc)
{
//   cout << "Changing table size " << size_table << " to " << new_size_table << " " << " unique: " << unique_elems << " elems:" << elems << endl;
   assert(new_size_table >= HASH_TABLE_INITIAL_TABLE_SIZE);

   const size_t to_alloc(new_size_table * sizeof(tuple_list*));
   tuple_list **new_table((tuple_list**)alloc->allocate_obj(to_alloc));
   memset(new_table, 0, to_alloc);
   prime = utils::previous_prime(new_size_table);

   for(size_t i(0); i < size_table; ++i) {
      tuple_list *ls(table[i]);

      while(ls) {
         tuple_list *saved_next(ls->next);
         const uint_val id(hash_field(ls->value));
         const uint_val idx(mod_hash(id));
         tuple_list *next(new_table[idx]);
         new_table[idx] = ls;
         ls->next = next;
         ls->prev = nullptr;
         ls = saved_next;
      }
   }

   // change pointers
   alloc->deallocate_obj((utils::byte*)table, size_table * sizeof(tuple_list*));
   table = new_table;
   size_table = new_size_table;
}

}
