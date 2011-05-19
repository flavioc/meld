
#ifndef SCHED_THREAD_ASSERT_HPP
#define SCHED_THREAD_ASSERT_HPP

namespace sched
{

void assert_thread_iteration(const size_t);
void assert_thread_pop_work(void);
void assert_thread_push_work(void);
void assert_thread_end_iteration(void);
void assert_thread_disable_work_count(void);

}

#endif
