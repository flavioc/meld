
#include "db/hash_table.hpp"
#include "db/node.hpp"

using namespace vm;

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
      default: assert(false); return 0;
   }
}

void
hash_table::insert(tuple *item)
{
   const uint_val id(hash_tuple(item));
   table_list *bucket(table + (id % size_table));
#ifdef MIXED_BUCKET
   bucket->push_back(item);
#else
   // try to find correct table_node
   table_node *node(bucket->head);
   for(; node != NULL; node = node->next) {
      switch(hash_type) {
         case FIELD_INT:
            if(FIELD_INT(node->field) == item->get_int(hash_argument))
               goto found;
            break;
         case FIELD_FLOAT:
            if(FIELD_FLOAT(node->field) == item->get_float(hash_argument))
               goto found;
            break;
         case FIELD_NODE:
            if(FIELD_NODE(node->field) == item->get_node(hash_argument))
               goto found;
            break;
         default:
            assert(false);
            break;
      }
   }

   // add new table node
   node = mem::allocator<table_node>().allocate(1);
   mem::allocator<table_node>().construct(node);

   node->field = item->get_field(hash_argument);
   node->next = bucket->head;
   bucket->head = node;
   bucket->size++;
   assert(node->ls.empty());

found:
   node->ls.push_back(item);
#endif
   added_so_far++;
}

void
hash_table::expand(void)
{
   const uint_val new_size_table(size_table * 2);
   table_list *new_table(alloc().allocate(new_size_table));

   check_limit *= 2;

   for(size_t i(0); i < new_size_table; ++i) {
      table_list *ls(new_table + i);
#ifdef MIXED_BUCKET
      alloc().construct(ls);
#else
      ls->head = NULL;
      ls->size = 0;
#endif
   }

   for(size_t i(0); i < size_table; ++i) {
      table_list *ls(table + i);

#ifdef MIXED_BUCKET
      for(table_list::iterator it(ls->begin()), end(ls->end()); it != end; ) {
         tuple *tpl(*it);
         it++;

         const uint_val id(hash_tuple(tpl));
         table_list *ls(new_table + (id % new_size_table));

         ls->push_back(tpl);
      }
#else
      for(table_node *node(ls->head); node != NULL; ) {
         table_node *next(node->next);
         const uint_val id(hash_field(node->field));
         table_list *bucket(new_table + (id % new_size_table));

         // add to bucket 'bucket'
         node->next = bucket->head;
         bucket->size++;
         bucket->head = node;

         node = next;
      }
#endif
   }

   // change pointers
   alloc().deallocate(table, size_table);
   table = new_table;
   size_table = new_size_table;
}

}
