
#ifndef UTILS_QUEUED_SPINLOCK_HPP
#define UTILS_QUEUED_SPINLOCK_HPP

#include <iostream>
#include <atomic>

#include "utils/utils.hpp"

namespace utils
{

struct qsl_entry
{
   // next processor in the queue that is waiting to enter section
   qsl_entry *next;

   // indicates whether the access to section has been granted to processor
   uint64_t state;
};

static inline void *xchg_64(void *ptr, void *x)
{
   __asm__ __volatile__("xchgq %0,%1"
         :"=r" (x)
         :"m" (*(volatile long long *)ptr), "0" ((unsigned long long) x)
         :"memory");

   return x;
}

class qspinlock
{
   private:
      qsl_entry* tail;

   public:

#define cpu_relax() asm volatile("pause\n": : :"memory")
#define barrier() asm volatile("": : :"memory")
      inline void lock(qsl_entry *ent)
      {
         qsl_entry *stail;

         ent->next = nullptr;
         ent->state = 0;
         stail = (qsl_entry*)xchg_64((void*)&tail, (void*)ent);
         if(!stail)
            return;
         stail->next = ent;
         barrier();
         while(!ent->state) cpu_relax();
         return;
      }

      inline bool try_lock(qsl_entry *ent)
      {
         qsl_entry *stail;

         ent->next = nullptr;
         ent->state = 0;
         stail = cmpxchg(&tail, nullptr, ent);
         if(!stail) return true;
         return false;
      }

      inline void unlock(qsl_entry *ent)
      {
         if(!ent->next) {
            if(cmpxchg(&tail, ent, nullptr) == ent)
               return;

            while(!ent->next) cpu_relax();
         }
         ent->next->state = 1;
      }

      explicit qspinlock(void): tail(nullptr) {}
};

}

#endif
