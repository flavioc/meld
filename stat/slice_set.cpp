
#include <fstream>
#include <iostream>

#include "stat/slice_set.hpp"
#include "vm/state.hpp"
#include "process/machine.hpp"
#include "stat/stat.hpp"
#include "utils/macros.hpp"

using namespace vm;
using namespace sched;
using namespace std;
using namespace utils;

namespace statistics
{

static void
write_header(ofstream& out, const string& header)
{
   csv_line line;

   line.set_header();

   line << header;

   for(size_t i(0); i < state::NUM_THREADS; ++i)
      line << (string("thread") + to_string<size_t>(i));

   line.print(out);
}
  
void
slice_set::write_general(const string& file, const string& title,
   print_fn fun) const
{
   ofstream out(file.c_str(), ios_base::out | ios_base::trunc);
   
   write_header(out, title);
   
   vector<iter> iters(state::NUM_THREADS, iter());

	 assert(slices.size() == state::NUM_THREADS);

   for(size_t i(0); i < state::NUM_THREADS; ++i) {
		  assert(num_slices == slices[i].size());
      iters[i] = slices[i].begin();
	 }
      
   for(size_t slice(0); slice < num_slices; ++slice) {
      csv_line line;
      line << to_string<unsigned long>(slice * SLICE_PERIOD);
      for(size_t proc(0); proc < state::NUM_THREADS; ++proc) {
         assert(iters[proc] != slices[proc].end());
         const statistics::slice& sl(*iters[proc]);
         CALL_MEMBER_FN(sl, fun)(line);
         iters[proc]++;
      }
      line.print(out);
   }
}

void
slice_set::write_state(const string& file) const
{
   write_general(file + ".state", "state", &slice::print_state);
}

void
slice_set::write_work_queue(const string& file) const
{
   write_general(file + ".work_queue", "workqueue", &slice::print_work_queue);
}

void
slice_set::write_processed_facts(const string& file) const
{
   write_general(file + ".processed_facts", "processedfacts", &slice::print_processed_facts);
}

void
slice_set::write_sent_facts(const string& file) const
{
   write_general(file + ".sent_facts", "sentfacts", &slice::print_sent_facts);
}

void
slice_set::write_stolen_nodes(const string& file) const
{
   write_general(file + ".stolen_nodes", "stolennodes", &slice::print_stolen_nodes);
}

void
slice_set::write_steal_requests(const string& file) const
{
   write_general(file + ".steal_requests", "stealrequests", &slice::print_steal_requests);
}

void
slice_set::write_priority_queue(const string& file) const
{
   write_general(file + ".priority_queue", "priorityqueue", &slice::print_priority_queue);
}

void
slice_set::write(const string& file, const scheduler_type type) const
{
   write_state(file);
   write_work_queue(file);
   write_processed_facts(file);
   write_sent_facts(file);
   write_stolen_nodes(file);
   write_steal_requests(file);
   if(is_priority_sched(type))
      write_priority_queue(file);
}
   
void
slice_set::beat_thread(const process_id id, slice& sl)
{
   state::MACHINE->get_scheduler(id)->write_slice(sl);
}

void
slice_set::beat(void)
{
   for(size_t i(0); i < state::NUM_THREADS; ++i) {
      slice sl;
      
      beat_thread(i, sl);
      
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
