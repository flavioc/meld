
#ifndef UTILS_MUTEX_HPP
#define UTILS_MUTEX_HPP

//#define USE_STD_MUTEX
//#define USE_SEMAPHORE
#define USE_SPINLOCK
//#define LOCK_STATISTICS

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
extern std::atomic<uint64_t> lock_count;
extern std::atomic<uint64_t> ok_locks;
extern std::atomic<uint64_t> failed_locks;
extern std::atomic<uint64_t> steal_locks;
extern std::atomic
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
#ifdef LOCK_STATISTICS
         lock_count++;
#endif
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
#ifdef LOCK_STATISTICS
         if(ret)
            ok_locks++;
         else
            failed_locks++;
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

