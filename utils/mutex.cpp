
#include <iostream>

#include "utils/mutex.hpp"

#ifdef LOCK_STATISTICS
namespace utils {
std::atomic<uint64_t> main_db_lock_ok, main_db_lock_fail;
std::atomic<uint64_t> node_lock_ok, node_lock_fail;
std::atomic<uint64_t> thread_lock_ok, thread_lock_fail;
std::atomic<uint64_t> database_lock_ok, database_lock_fail;
std::atomic<uint64_t> normal_lock_ok, normal_lock_fail;
std::atomic<uint64_t> coord_normal_lock_ok, coord_normal_lock_fail;
std::atomic<uint64_t> priority_lock_ok, priority_lock_fail;
std::atomic<uint64_t> coord_priority_lock_ok, coord_priority_lock_fail;
std::atomic<uint64_t> schedule_next_lock_ok, schedule_next_lock_fail;
std::atomic<uint64_t> add_priority_lock_ok, add_priority_lock_fail;
std::atomic<uint64_t> set_priority_lock_ok, set_priority_lock_fail;
std::atomic<uint64_t> set_moving_lock_ok, set_moving_lock_fail;
std::atomic<uint64_t> set_static_lock_ok, set_static_lock_fail;
std::atomic<uint64_t> set_affinity_lock_ok, set_affinity_lock_fail;
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
#endif
}
