
#ifndef UTILS_MUTEX_HPP
#define UTILS_MUTEX_HPP

//#define USE_STD_MUTEX
//#define USE_SEMAPHORE
#define USE_SPINLOCK

#ifdef LOCK_STATISTICS
#include <atomic>
#endif

#include <mutex>
#ifdef USE_SEMAPHORE
#include <semaphore.h>
#elif defined(USE_SPINLOCK)
#include "utils/spinlock.hpp"
#endif

namespace utils
{

#ifdef LOCK_STATISTICS
extern std::atomic<uint64_t> internal_ok_locks;
extern std::atomic<uint64_t> internal_failed_locks;
extern std::atomic<uint64_t> steal_locks;
extern std::atomic<uint64_t> internal_locks;
extern std::atomic<uint64_t> ready_lock;
extern std::atomic<uint64_t> sched_lock;
extern std::atomic<uint64_t> add_lock;
extern std::atomic<uint64_t> check_lock;
extern std::atomic<uint64_t> prio_lock;
#define LOCK_STAT(name) utils::name++
#else
#define LOCK_STAT(name)
#endif

class mutex
{
   private:

#ifdef USE_STD_MUTEX
      std::mutex mtx;
#endif
#ifdef USE_SEMAPHORE
      sem_t s;
#endif
#ifdef USE_SPINLOCK
      utils::spinlock lck;
#endif

   public:

      static void print_statistics(void);

      inline void lock(void) {
#ifdef USE_STD_MUTEX
         mtx.lock();
#elif defined(USE_SEMAPHORE)
         sem_wait(&s);
#elif defined(USE_SPINLOCK)
         lck.lock();
#endif
      }
      inline void unlock(void) {
#ifdef USE_STD_MUTEX
         mtx.unlock();
#elif defined(USE_SEMAPHORE)
         sem_post(&s);
#elif defined(USE_SPINLOCK)
         lck.unlock();
#endif
      }
      inline bool try_lock(void) {
#ifdef USE_STD_MUTEX
         const bool ret = mtx.try_lock();
#elif defined(USE_SEMAPHORE)
         const bool ret = (sem_trywait(&s) == 0);
#elif defined(USE_SPINLOCK)
         const bool ret = lck.try_lock();
#endif
         return ret;
      }

#ifdef USE_SEMAPHORE
      explicit mutex(void) {
         sem_init(&s, 1, 1);
      }
#endif
};

}

#endif

