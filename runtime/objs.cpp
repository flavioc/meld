
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
   } else {
      db::node *node((db::node*)p);
      for(auto it(All->DATABASE->nodes_begin()); it != All->DATABASE->nodes_end(); ++it) {
         if(it->second == node) {
            cout << "GOT NODE!!!!!!!!\n";
         }
      }
      p->refs++;
   }
}

void
do_decrement_runtime(const vm::tuple_field& f, const vm::field_type t
#ifdef GC_NODES
      , candidate_gc_nodes& gc_nodes
#endif
      )
{
   runtime::ref_base *p((ref_base*)FIELD_PTR(f));

   if(!p)
      return;

   switch(t) {
      case FIELD_LIST:
         assert(p->refs > 0);
         p->refs--;
         if(p->refs == 0)
            ((runtime::cons*)p)->destroy(gc_nodes);
         break;
      case FIELD_STRUCT:
         assert(p->refs > 0);
         p->refs--;
         if(p->refs == 0)
            ((runtime::struct1*)p)->destroy(gc_nodes);
         break;
      case FIELD_STRING:
         assert(p->refs > 0);
         p->refs--;
         if(p->refs == 0)
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
               node->refs--;
               if(node->refs == 0) {
                  if(node->garbage_collect())
                     gc_nodes.insert((vm::node_val)node);
               }
            }
         }
#endif
         break;
     default: assert(false); break;
   }
}

}
