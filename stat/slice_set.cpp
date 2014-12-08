
#include <fstream>
#include <iostream>

#include "stat/slice_set.hpp"
#include "vm/state.hpp"
#include "machine.hpp"
#include "stat/stat.hpp"
#include "utils/macros.hpp"

using namespace vm;
using namespace sched;
using namespace std;
using namespace utils;

namespace statistics
{

void
slice_set::write(const string& file, vm::all *all) const
{
   write_general(file + ".state", "state", all, [](const slice& sl) {
      switch(sl.state) {
      case NOW_ACTIVE:
         return to_string<size_t>(sl.work_queue);
      case NOW_IDLE:
         return std::string("i");
      case NOW_SCHED:
         return std::string("s");
      case NOW_ROUND:
         return std::string("r");
      default: assert(false);
   }});
   write_general(file + ".derived_facts", "derivedfacts", all, [](const slice& sl) { return sl.derived_facts; });
   write_general(file + ".consumed_facts", "consumedfacts", all, [](const slice& sl) { return sl.consumed_facts; });
   write_general(file + ".rules_run", "rulesrun", all, [](const slice& sl) { return sl.rules_run; });
   write_general(file + ".stolen_nodes", "stolennodes", all, [](const slice& sl) { return sl.stolen_nodes; });
   write_general(file + ".sent_facts_same_thread", "sentfactssamethread", all,
         [](const slice& sl) { return sl.sent_facts_same_thread; });
   write_general(file + ".sent_facts_other_thread", "sentfactsotherthread", all,
         [](const slice& sl) { return sl.sent_facts_other_thread; });
   write_general(file + ".sent_facts_other_thread_now", "sentfactsotherthreadnow", all,
         [](const slice& sl) { return sl.sent_facts_other_thread_now; });
   write_general(file + ".priority_nodes_thread", "prioritynodesthread", all,
         [](const slice& sl) { return sl.priority_nodes_thread; });
   write_general(file + ".priority_nodes_others", "prioritynodesothers", all,
         [](const slice& sl) { return sl.priority_nodes_others; });
   write_general(file + ".bytes_used", "bytesused", all,
         [](const slice& sl) { return sl.bytes_used; });
   write_general(file + ".node_lock_ok", "nodelockok", all,
         [](const slice& sl) { return sl.node_lock_ok; });
   write_general(file + ".node_lock_fail", "nodelockfail", all,
         [](const slice& sl) { return sl.node_lock_fail; });
   write_general(file + ".thread_transactions", "threadtransactions", all,
         [](const slice& sl) { return sl.thread_transactions; });
   write_general(file + ".all_transactions", "alltransactions", all,
         [](const slice& sl) { return sl.all_transactions; });
   write_general(file + ".node_difference", "node_difference", all,
         [](const slice& sl) { return sl.node_difference; });
}
   
void
slice_set::beat_thread(const process_id id, slice& sl, vm::all *all)
{
   if(all->SCHEDS[id])
      all->SCHEDS[id]->write_slice(sl);
}

void
slice_set::beat(vm::all *all)
{
   for(size_t i(0); i < all->NUM_THREADS; ++i) {
      slice sl;
      
      beat_thread(i, sl, all);
      
      slices[i].push_back(sl);
   }
   
   ++num_slices;
}
   
slice_set::slice_set(const size_t num_threads):
   num_slices(0),
   slices(num_threads, list_slices())
{
}

}
