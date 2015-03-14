
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
   const uint_val id(hash_tuple(item, pred));
   tuple_list *bucket(table + mod_hash(id));
   bucket->push_front(item);
   return bucket->get_size();
}

void
hash_table::change_table(const std::uint16_t new_size_table, const vm::predicate *pred)
{
   assert(new_size_table >= HASH_TABLE_INITIAL_TABLE_SIZE);

   tuple_list *new_table(allocator().allocate(new_size_table));
   memset(new_table, 0, sizeof(tuple_list)*new_size_table);
   prime = utils::previous_prime(new_size_table);

   for(size_t i(0); i < size_table; ++i) {
      tuple_list *ls(table + i);

      for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end; ) {
         vm::tuple *tpl(*it);
         it++;

         const uint_val id(hash_tuple(tpl, pred));
         tuple_list *ls(new_table + mod_hash(id));

         ls->push_back(tpl);
      }
   }

   // change pointers
   allocator().deallocate(table, size_table);
   table = new_table;
   size_table = new_size_table;
}

}
