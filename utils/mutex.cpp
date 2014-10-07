
#include <iostream>

#include "utils/mutex.hpp"

#ifdef LOCK_STATISTICS
namespace utils {
std::atomic<uint64_t> internal_ok_locks;
std::atomic<uint64_t> internal_failed_locks;
std::atomic<uint64_t> steal_locks;
std::atomic<uint64_t> internal_locks;
std::atomic<uint64_t> ready_lock;
std::atomic<uint64_t> sched_lock;
std::atomic<uint64_t> add_lock;
std::atomic<uint64_t> check_lock;
std::atomic<uint64_t> prio_lock;
}
#endif

using std::cerr;
using std::endl;

void
utils::mutex::print_statistics(void)
{
#ifdef LOCK_STATISTICS
   cerr << "ok,failed,steal,internal,ready,sched,add,check,prio" << endl;
   cerr << internal_ok_locks << "," << internal_failed_locks << "," <<
      steal_locks << "," << internal_locks << "," << ready_lock << "," <<
      sched_lock << "," << add_lock << "," << check_lock << "," <<
      prio_lock << endl;
#endif
}
