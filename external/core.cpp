
#include <algorithm>

#include "external/core.hpp"
#include "thread/threads.hpp"
#include "db/database.hpp"

using namespace sched;
using namespace db;
using namespace vm;
using namespace std;

namespace vm
{
namespace external
{

argument
node_priority(EXTERNAL_ARG(id))
{
   DECLARE_NODE(id);
   priority_t ret(0.0);
   node *tn(NULL);

#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
   assert(n != NULL);
#endif

   if(tn) {
      ret = tn->get_priority();
   } else
      ret = no_priority_value();

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

argument
is_moving(EXTERNAL_ARG(id))
{
   bool_val ret(false);
   DECLARE_NODE(id);

   node *tn(NULL);
#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
#endif

   ret = tn->is_static();
   RETURN_BOOL(ret);
}

argument
is_static(EXTERNAL_ARG(id))
{
   bool_val ret(false);
   DECLARE_NODE(id);

   node *tn(NULL);
#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
#endif

   ret = tn->is_moving();
   RETURN_BOOL(ret);
}

argument
partition_vertical(EXTERNAL_ARG(x), EXTERNAL_ARG(y), EXTERNAL_ARG(lx), EXTERNAL_ARG(ly))
{
   DECLARE_INT(x);
   DECLARE_INT(y);
   DECLARE_INT(lx);
   DECLARE_INT(ly);

   const int_val pos(ly * y + x);
   const int_val total(ly * lx);
   assert(All->NUM_THREADS > 0);
   const int_val part(std::max((int_val)1, (int_val)(total / All->NUM_THREADS)));

   // if it's not divisible by NUM_THREADS, last thread will get the rest.
   const int_val ret(std::max(pos / part, (int_val)All->NUM_THREADS-1));

   RETURN_INT(ret);
}

}
}
