
#ifndef UTILS_MUTEX_HPP
#define UTILS_MUTEX_HPP

#define USE_STD_MUTEX

#ifdef USE_STD_MUTEX
#include <mutex>
#else
#include <semaphore.h>
#endif

namespace utils
{

//#define LOCK_STATISTICS

extern std::atomic<uint64_t> lock_count;
extern std::atomic<uint64_t> ok_locks;
extern std::atomic<uint64_t> failed_locks;

class mutex
{
   private:

#ifdef USE_STD_MUTEX
      std::mutex mtx;
#else
      sem_t s;
#endif

   public:

      static void print_statistics(void);

#ifdef USE_STD_MUTEX
      inline void lock(void) {
#ifdef LOCK_STATISTICS
         lock_count++;
#endif
         mtx.lock();
      }
      inline void unlock(void) { mtx.unlock(); }
      inline bool try_lock(void) {
         const bool ret = mtx.try_lock();
#ifdef LOCK_STATISTICS
         if(ret)
            ok_locks++;
         else
            failed_locks++;
#endif
         return ret;
      }
#else
      inline void lock(void) { sem_wait(&s); }
      inline void unlock(void) { sem_post(&s); }

      explicit mutex(void) {
         sem_init(&s, 1, 1);
      }
#endif
};

}

#endif

