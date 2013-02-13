
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

   if(prio_db != NULL) {
      node *n(prio_db->find_node(id));
      assert(n != NULL);
      thread_intrusive_node *tn((thread_intrusive_node *)n);

      ret = tn->get_float_priority_level();
   }

   RETURN_FLOAT(ret);
}

argument
cpu_id(EXTERNAL_ARG(id))
{
   int_val ret(0);
   DECLARE_NODE(id);

#if 0
   sched::base *sched((sched::base*)pthread_getspecific(sched_key));

   assert(sched != NULL);

   ret = (int_val)sched->get_id();
#endif
   if(prio_db != NULL) {
      node *n(prio_db->find_node(id));
      assert(n != NULL);
      thread_intrusive_node *tn((thread_intrusive_node *)n);

      ret = tn->get_owner()->get_id();
   }

   RETURN_INT(ret);
}

}
}
