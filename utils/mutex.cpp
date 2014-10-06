
#include <iostream>

#include "utils/mutex.hpp"

std::atomic<uint64_t> utils::lock_count(0);
std::atomic<uint64_t> utils::ok_locks(0);
std::atomic<uint64_t> utils::failed_locks(0);

using std::cout;
using std::endl;

void
utils::mutex::print_statistics(void)
{
   cout << "Total locks acquired: " << lock_count << endl;
   cout << "Trylocks that succeeded: " << ok_locks << endl;
   cout << "Trylocks that failed: " << failed_locks << endl;
}
