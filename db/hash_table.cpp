
#include <iostream>

#include "db/hash_table.hpp"
#include "db/node.hpp"

using namespace vm;
using namespace std;

namespace db
{

uint_val
hash_table::hash_field(const tuple_field field) const
{
   switch(hash_type) {
      case FIELD_INT: return (uint_val)FIELD_INT(field);
      case FIELD_FLOAT: return (uint_val)FIELD_FLOAT(field);
      case FIELD_NODE:
#ifdef USE_REAL_NODES
         return ((node*)FIELD_PTR(field))->get_id();
#else
         return FIELD_NODE(field);
#endif
      case FIELD_LIST:
         if(FIELD_PTR(field) == 0)
            return 0;
         else
            return 1;
      default: assert(false); return 0;
   }
}

size_t
hash_table::insert(vm::tuple *item)
{
   const uint_val id(hash_tuple(item));
   table_list *bucket(table + (id % size_table));
   bucket->push_back(item);
   return bucket->get_size();
}

size_t
hash_table::insert_front(vm::tuple *item)
{
   const uint_val id(hash_tuple(item));
   table_list *bucket(table + (id % size_table));
   bucket->push_front(item);
   return bucket->get_size();
}

void
hash_table::change_table(const size_t new_size_table)
{
   assert(new_size_table >= HASH_TABLE_INITIAL_TABLE_SIZE);

   table_list *new_table(alloc().allocate(new_size_table));
   memset(new_table, 0, sizeof(table_list)*new_size_table);

   for(size_t i(0); i < size_table; ++i) {
      table_list *ls(table + i);

      for(table_list::iterator it(ls->begin()), end(ls->end()); it != end; ) {
         vm::tuple *tpl(*it);
         it++;

         const uint_val id(hash_tuple(tpl));
         table_list *ls(new_table + (id % new_size_table));

         ls->push_back(tpl);
      }
   }

   // change pointers
   alloc().deallocate(table, size_table);
   table = new_table;
   size_table = new_size_table;
}

}
