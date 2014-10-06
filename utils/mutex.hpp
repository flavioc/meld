
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

class mutex
{
   private:

#ifdef USE_STD_MUTEX
      std::mutex mtx;
#else
      sem_t s;
#endif

   public:

#ifdef USE_STD_MUTEX
      inline void lock(void) { mtx.lock(); }
      inline void unlock(void) { mtx.unlock(); }
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

