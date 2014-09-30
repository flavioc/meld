
#include "external/core.hpp"
#include "thread/prio.hpp"
#include "db/database.hpp"

using namespace sched;
using namespace db;
using namespace std;

namespace vm
{
namespace external
{

argument
node_priority(EXTERNAL_ARG(id))
{
   DECLARE_NODE(id);
   float_val ret(0.0);
   node *n(NULL);

#ifdef USE_REAL_NODES
   n = (node*)id;
#else
   n = All->DATABASE->find_node(id);
   assert(n != NULL);
#endif

   if(n) {
      thread_intrusive_node *tn((thread_intrusive_node *)n);
      ret = tn->get_priority_level();
   } else
      ret = 0.0;

   RETURN_FLOAT(ret);
}

argument
cpu_id(EXTERNAL_ARG(id))
{
   int_val ret(0);
   DECLARE_NODE(id);

   node *n(NULL);
#ifdef USE_REAL_NODES
   n = (node*)id;
#else
   n = All->DATABASE->find_node(id);
   assert(n != NULL);
#endif

   if(n)
      ret = n->get_owner()->get_id();
   else
      ret = 0;

   RETURN_INT(ret);
}

}
}
