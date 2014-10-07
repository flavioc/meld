
#include <iostream>

#include "utils/mutex.hpp"

#ifdef LOCK_STATISTICS
std::atomic<uint64_t> utils::lock_count(0);
std::atomic<uint64_t> utils::ok_locks(0);
std::atomic<uint64_t> utils::failed_locks(0);
#endif

using std::cerr;
using std::endl;

void
utils::mutex::print_statistics(void)
{
#ifdef LOCK_STATISTICS
   cerr << "Total locks acquired: " << lock_count << endl;
   cerr << "Trylocks that succeeded: " << ok_locks << endl;
   cerr << "Trylocks that failed: " << failed_locks << endl;
#endif
}
