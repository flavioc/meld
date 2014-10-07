
#ifndef SCHED_THREAD_ASSERT_HPP
#define SCHED_THREAD_ASSERT_HPP

#include <cstdlib>

namespace sched
{

#ifdef ASSERT_THREADS
void assert_thread_iteration(const size_t);
void assert_thread_pop_work(void);
void assert_thread_push_work(void);
void assert_thread_end_iteration(void);
void assert_thread_disable_work_count(void);
#else
inline void assert_thread_iteration(const size_t) {}
inline void assert_thread_pop_work(void) {}
inline void assert_thread_push_work(void) {}
inline void assert_thread_end_iteration(void) {}
inline void assert_thread_disable_work_count(void) {}
#endif

}

#endif
