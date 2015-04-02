
#include "db/node.hpp"
#include "db/database.hpp"
#include "runtime/objs.hpp"

using namespace std;
using namespace vm;
using namespace db;

namespace runtime
{

void
do_increment_runtime(const vm::tuple_field& f, const vm::field_type t)
{
   runtime::ref_base *p((ref_base*)FIELD_PTR(f));
   if(!p)
      return;

   if(t == FIELD_NODE) {
      db::node *node((db::node*)FIELD_NODE(f));
      if(!All->DATABASE->is_initial_node(node))
         p->refs++;
   } else
      p->refs++;
}

void
do_decrement_runtime(const vm::tuple_field& f, const vm::type *t, candidate_gc_nodes& gc_nodes)
{
   (void)gc_nodes;

   runtime::ref_base *p((ref_base*)FIELD_PTR(f));

   if(!p)
      return;

   switch(t->get_type()) {
      case FIELD_LIST:
         assert(p->refs > 0);
         if(--(p->refs) == 0)
            ((runtime::cons*)p)->destroy((list_type*)t, gc_nodes);
         break;
      case FIELD_STRUCT:
         assert(p->refs > 0);
         if(--(p->refs) == 0)
            ((runtime::struct1*)p)->destroy((struct_type*)t, gc_nodes);
         break;
      case FIELD_ARRAY:
         assert(p->refs > 0);
         if(--(p->refs) == 0) {
            array_type *tt((array_type*)t);
            ((runtime::array*)p)->destroy(tt->get_base(), gc_nodes);
         }
         break;
      case FIELD_SET:
         assert(p->refs > 0);
         if(--(p->refs) == 0) {
            set_type *tt((set_type*)t);
            ((runtime::set*)p)->destroy(tt->get_base(), gc_nodes);
         }
         break;
      case FIELD_STRING:
         assert(p->refs > 0);
         if(--(p->refs) == 0)
            ((runtime::rstring*)p)->destroy();
         break;
      case FIELD_NODE:
#ifdef GC_NODES
         {
            if(All->DATABASE->is_deleting())
               return;
            db::node *node((db::node*)FIELD_PTR(f));
            assert(node->refs > 0);
            if(!All->DATABASE->is_initial_node(node)) {
               if(--(node->refs) == 0) {
                  if(node->garbage_collect())
                     gc_nodes.insert((vm::node_val)node);
               }
            }
         }
#endif
         break;
     default: abort(); break;
   }
}

}
