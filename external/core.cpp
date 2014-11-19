
#include "external/core.hpp"
#include "thread/threads.hpp"
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
   node *tn(NULL);

#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
   assert(n != NULL);
#endif

   if(tn) {
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
#endif
   assert(n != NULL);

   if(n)
      ret = n->get_owner()->get_id();
   else
      ret = 0;

   RETURN_INT(ret);
}

argument
cpu_static(EXTERNAL_ARG(id))
{
   int_val ret(0);
   DECLARE_NODE(id);
   
   node *tn(NULL);
#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
#endif
   assert(tn != NULL);

   threads_sched *owner(static_cast<threads_sched*>(tn->get_owner()));

   ret = owner->num_static_nodes();

   RETURN_INT(ret);
}

}
}
