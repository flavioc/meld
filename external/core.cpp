
#include <algorithm>

#include "external/core.hpp"
#include "thread/thread.hpp"
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
priority(EXTERNAL_ARG(id))
{
   DECLARE_NODE(id);
   priority_t ret(0.0);
   node *tn(nullptr);

#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
   assert(n != nullptr);
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

   node *n(nullptr);
#ifdef USE_REAL_NODES
   n = (node*)id;
#else
   n = All->DATABASE->find_node(id);
#endif
   assert(n != nullptr);

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
   
   node *tn(nullptr);
#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
#endif
   assert(tn != nullptr);

   thread *owner(static_cast<thread*>(tn->get_owner()));

   ret = owner->num_static_nodes();

   RETURN_INT(ret);
}

argument
is_moving(EXTERNAL_ARG(id))
{
   bool_val ret(false);
   DECLARE_NODE(id);

   node *tn(nullptr);
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

   node *tn(nullptr);
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
   const int_val ret(std::min(pos / part, (int_val)All->NUM_THREADS-1));

   RETURN_THREAD(All->SCHEDS[ret]);
}

argument
partition_horizontal(EXTERNAL_ARG(x), EXTERNAL_ARG(y), EXTERNAL_ARG(lx), EXTERNAL_ARG(ly))
{
   DECLARE_INT(x);
   DECLARE_INT(y);
   DECLARE_INT(lx);
   DECLARE_INT(ly);

   const int_val pos(lx * x + y);
   const int_val total(ly * lx);
   assert(All->NUM_THREADS > 0);
   const int_val part(std::max((int_val)1, (int_val)(total / All->NUM_THREADS)));

   // if it's not divisible by NUM_THREADS, last thread will get the rest.
   const int_val ret(std::min(pos / part, (int_val)All->NUM_THREADS-1));

   RETURN_THREAD(All->SCHEDS[ret]);
}

argument
partition_grid(EXTERNAL_ARG(x), EXTERNAL_ARG(y), EXTERNAL_ARG(lx), EXTERNAL_ARG(ly))
{
   DECLARE_INT(x);
   DECLARE_INT(y);
   DECLARE_INT(lx);
   DECLARE_INT(ly);

   assert(All->NUM_THREADS > 0);

   // divide grid by fl x fl blocks
   int_val fl(max(1, (int_val)round(sqrt((double)All->NUM_THREADS))));
   while(fl * fl < (int_val)All->NUM_THREADS && fl <= lx -1 && fl <= ly - 1)
      fl++;
   const int_val total_blocks(fl * fl);
   // find block width and height
   const int_val height(std::max(1, (int_val)((float_val)lx / fl)));
   const int_val width(std::max(1, (int_val)((float_val)ly / fl)));
   // find block coordinates
   const int_val block_x(std::min(fl - 1, (int_val)(x / height)));
   const int_val block_y(std::min(fl - 1, (int_val)(y / width)));
   int_val block_coord_y;
   if(y >= fl * width)
      block_coord_y = width - 1;
   else
      block_coord_y = y % width;

   // find block number
   int_val block_id(block_x * fl);
   if(block_x % 2 == 0) {
      // even
      block_id += block_y;
   } else {
      // odd
      block_id += fl - block_y - 1;
      block_coord_y = width - block_coord_y - 1;
   }
   const int_val length_node(block_id * width + block_coord_y);
   const int_val total_length(width * total_blocks);
   const int_val length_per_thread(max(1, total_length / (int_val)All->NUM_THREADS));
   const int_val ret(min((int_val)(All->NUM_THREADS-1), length_node / length_per_thread));

   RETURN_THREAD(All->SCHEDS[ret]);
}

argument
queue_size(EXTERNAL_ARG(id))
{
   DECLARE_NODE(id);

   node *tn(nullptr);
#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
#endif

   auto *owner(tn->get_owner());
   const int_val size((int_val)owner->queue_size());

   RETURN_INT(size);
}

argument
facts_proved(EXTERNAL_ARG(id))
{
   DECLARE_NODE(id);

   node *tn(nullptr);
#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
#endif
   (void)tn;

   int_val total(0);
   // this is actually implemented by an instruction
   abort();
   RETURN_INT(total);
}

argument
facts_consumed(EXTERNAL_ARG(id))
{
   DECLARE_NODE(id);

   node *tn(nullptr);
   (void)tn;
#ifdef USE_REAL_NODES
   tn = (node*)id;
#else
   tn = All->DATABASE->find_node(id);
#endif

   int_val total(0);
   // this is actually implemented by an instruction
   abort();
   RETURN_INT(total);
}

}
}
