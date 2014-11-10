
#include <iostream>

#include "utils/mutex.hpp"

#ifdef LOCK_STATISTICS
namespace utils {
std::atomic<uint64_t> main_db_lock_ok(0), main_db_lock_fail(0);
std::atomic<uint64_t> node_lock_ok(0), node_lock_fail(0);
std::atomic<uint64_t> thread_lock_ok(0), thread_lock_fail(0);
std::atomic<uint64_t> database_lock_ok(0), database_lock_fail(0);
std::atomic<uint64_t> normal_lock_ok(0), normal_lock_fail(0);
std::atomic<uint64_t> coord_normal_lock_ok(0), coord_normal_lock_fail(0);
std::atomic<uint64_t> priority_lock_ok(0), priority_lock_fail(0);
std::atomic<uint64_t> coord_priority_lock_ok(0), coord_priority_lock_fail(0);
std::atomic<uint64_t> schedule_next_lock_ok(0), schedule_next_lock_fail(0);
std::atomic<uint64_t> add_priority_lock_ok(0), add_priority_lock_fail(0);
std::atomic<uint64_t> set_priority_lock_ok(0), set_priority_lock_fail(0);
std::atomic<uint64_t> set_moving_lock_ok(0), set_moving_lock_fail(0);
std::atomic<uint64_t> set_static_lock_ok(0), set_static_lock_fail(0);
std::atomic<uint64_t> set_affinity_lock_ok(0), set_affinity_lock_fail(0);
std::atomic<uint64_t> heap_operations(0);
}
#endif

using std::cerr;
using std::endl;

void
utils::mutex::print_statistics(void)
{
#ifdef LOCK_STATISTICS
#define SHOW(NAME) cerr << #NAME ": " << NAME ## _fail << "\t/\t" << (NAME ## _ok + NAME ## _fail) << endl
   SHOW(main_db_lock);
   SHOW(node_lock);
   SHOW(thread_lock);
   SHOW(database_lock);
   SHOW(normal_lock);
   SHOW(coord_normal_lock);
   SHOW(priority_lock);
   SHOW(coord_priority_lock);
   SHOW(schedule_next_lock);
   SHOW(add_priority_lock);
   SHOW(set_priority_lock);
   SHOW(set_moving_lock);
   SHOW(set_static_lock);
   SHOW(set_affinity_lock);
   cerr << "heap_operations: " << heap_operations << endl;
#endif
}
