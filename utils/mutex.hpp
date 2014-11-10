
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

#ifdef LOCK_STATISTICS
#define MUTEX_LOCK(LCK, ARG, STAT) do { if((LCK).try_lock1(LOCK_STACK_USE(ARG))) { utils::STAT ## _ok++; } else { utils::STAT ## _fail++; (LCK).lock1(LOCK_STACK_USE(ARG)); }} while(false)
#define LOCKING_STAT(NAME) utils::NAME++
#else
#define MUTEX_LOCK(LCK, ARG, STAT) ((LCK).lock1(LOCK_STACK_USE(ARG)))
#define LOCKING_STAT(NAME)
#endif
#define MUTEX_UNLOCK(LCK, ARG) ((LCK).unlock1(LOCK_STACK_USE(ARG)))

namespace utils
{

#ifdef LOCK_STATISTICS
extern std::atomic<uint64_t> main_db_lock_ok, main_db_lock_fail;
extern std::atomic<uint64_t> node_lock_ok, node_lock_fail;
extern std::atomic<uint64_t> thread_lock_ok, thread_lock_fail;
extern std::atomic<uint64_t> database_lock_ok, database_lock_fail;
extern std::atomic<uint64_t> normal_lock_ok, normal_lock_fail;
extern std::atomic<uint64_t> coord_normal_lock_ok, coord_normal_lock_fail;
extern std::atomic<uint64_t> priority_lock_ok, priority_lock_fail;
extern std::atomic<uint64_t> coord_priority_lock_ok, coord_priority_lock_fail;
extern std::atomic<uint64_t> schedule_next_lock_ok, schedule_next_lock_fail;
extern std::atomic<uint64_t> add_priority_lock_ok, add_priority_lock_fail;
extern std::atomic<uint64_t> set_priority_lock_ok, set_priority_lock_fail;
extern std::atomic<uint64_t> set_moving_lock_ok, set_moving_lock_fail;
extern std::atomic<uint64_t> set_static_lock_ok, set_static_lock_fail;
extern std::atomic<uint64_t> set_affinity_lock_ok, set_affinity_lock_fail;
extern std::atomic<uint64_t> heap_operations;
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

      inline void lock1(LOCK_ARGUMENT) {
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
      inline void unlock1(LOCK_ARGUMENT) {
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
      inline bool try_lock1(LOCK_ARGUMENT) {
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

class lock_guard1
{
   public:

      utils::mutex *m;
      LOCK_STACK(data);

      explicit lock_guard1(utils::mutex& mtx): m(&mtx) {
      }

      ~lock_guard1(void) { m->unlock1(LOCK_STACK_USE(data)); }
};

#ifdef LOCK_STATISTICS
#define MUTEX_LOCK_GUARD_NAME(NAME, LCK, STAT) utils::lock_guard1 NAME(LCK); do { if((LCK).try_lock1(LOCK_STACK_USE((NAME).data))) { utils::STAT ## _ok++; } else { utils::STAT ## _fail++; (LCK).lock1(LOCK_STACK_USE((NAME).data)); }} while(false)
#define MUTEX_LOCK_GUARD(LCK, STAT) MUTEX_LOCK_GUARD_NAME(l1, LCK, STAT)
#define MUTEX_LOCK_GUARD_FLAG(LCK, STAT1, STAT2) utils::lock_guard1 l1(LCK); do { if((LCK.try_lock1(LOCK_STACK_USE(l1.data)))) { if(lock_stat_use1) utils::STAT1 ## _ok++; else utils::STAT2 ## _ok++;} else { if(lock_stat_use1) utils::STAT1 ## _fail++; else utils::STAT2 ## _fail++; (LCK).lock1(LOCK_STACK_USE(l1.data)); }} while(false)
#define LOCKING_STAT_FLAG , bool lock_stat_use1 = true
#define LOCKING_STAT_FLAG_FALSE , false
#else
#define MUTEX_LOCK_GUARD_NAME(NAME, LCK, STAT) utils::lock_guard1 NAME(LCK); (LCK).lock1(LOCK_STACK_USE((NAME).data))
#define MUTEX_LOCK_GUARD(LCK, STAT) MUTEX_LOCK_GUARD_NAME(l1, LCK, STAT)
#define MUTEX_LOCK_GUARD_FLAG(LCK, STAT1, STAT2) MUTEX_LOCK_GUARD_NAME(l1, LCK, STAT1)
#define LOCKING_STAT_FLAG
#define LOCKING_STAT_FLAG_FALSE
#endif

}

#endif

