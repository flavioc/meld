
#ifndef UTILS_MUTEX_HPP
#define UTILS_MUTEX_HPP

#ifdef LOCK_STATISTICS
#include <atomic>
#endif

#include <mutex>
#include "utils/queued_spinlock.hpp"
#ifdef USE_SEMAPHORE
#include <semaphore.h>
#elif defined(USE_SPINLOCK)
#include "utils/spinlock.hpp"
#endif
#ifdef USE_QUEUED_SPINLOCK
#define LOCK_ARGUMENT utils::qsl_entry *ent
#define LOCK_STACK(NAME) utils::qsl_entry NAME;
#define LOCK_STACK_USE(NAME) &(NAME)
#define LOCK_ARGUMENT_NAME ent
#else
#define LOCK_ARGUMENT void
#define LOCK_ARGUMENT_NAME 
#define LOCK_STACK(NAME)
#define LOCK_STACK_USE(NAME)
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
#ifdef USE_QUEUED_SPINLOCK
      utils::qspinlock lck;
#endif

   public:

      static void print_statistics(void);

      inline void lock(LOCK_ARGUMENT) {
#ifdef USE_STD_MUTEX
         mtx.lock();
#elif defined(USE_SEMAPHORE)
         sem_wait(&s);
#elif defined(USE_SPINLOCK)
         lck.lock();
#elif defined(USE_QUEUED_SPINLOCK)
         lck.lock(ent);
#endif
      }
      inline void unlock(LOCK_ARGUMENT) {
#ifdef USE_STD_MUTEX
         mtx.unlock();
#elif defined(USE_SEMAPHORE)
         sem_post(&s);
#elif defined(USE_SPINLOCK)
         lck.unlock();
#elif defined(USE_QUEUED_SPINLOCK)
         lck.unlock(ent);
#endif
      }
      inline bool try_lock(LOCK_ARGUMENT) {
#ifdef USE_STD_MUTEX
         const bool ret = mtx.try_lock();
#elif defined(USE_SEMAPHORE)
         const bool ret = (sem_trywait(&s) == 0);
#elif defined(USE_SPINLOCK)
         const bool ret = lck.try_lock();
#elif defined(USE_QUEUED_SPINLOCK)
         const bool ret = lck.try_lock(ent);
#endif
         return ret;
      }

#ifdef USE_SEMAPHORE
      explicit mutex(void) {
         sem_init(&s, 1, 1);
      }
#endif
};

class lock_guard
{
   private:

      utils::mutex *m;
      LOCK_STACK(data);

   public:

      explicit lock_guard(utils::mutex& mtx): m(&mtx) {
         m->lock(LOCK_STACK_USE(data));
      }

      ~lock_guard(void) { m->unlock(LOCK_STACK_USE(data)); }
};

}

#endif

