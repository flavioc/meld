
#ifndef UTILS_QUEUED_SPINLOCK_HPP
#define UTILS_QUEUED_SPINLOCK_HPP

#include <iostream>

namespace utils
{

struct qsl_entry
{

   // next processor in the queue that is waiting to enter section
   std::atomic<qsl_entry*> next;

   // indicates whether the access to section has been granted to processor
   std::atomic<long long> state;

};

class qspinlock
{
   private:
      qsl_entry* tail;

   public:

#define cpu_relax() asm volatile("pause\n": : :"memory")
      inline void lock(qsl_entry *ent)
      {
         ent->next = NULL;
         ent->state = 1;
         qsl_entry *old(__sync_lock_test_and_set(&tail, ent));
         if(old == NULL)
            return;
         old->next = ent;
         while(ent->state.load() == 1) {
            cpu_relax();
         }
      }

      inline bool try_lock(qsl_entry *ent)
      {
         if(tail == NULL) {
            lock(ent);
            return true;
         }
         return false;
      }

      inline void unlock(qsl_entry *entry)
      {
         if(__sync_val_compare_and_swap(&tail, entry, NULL) == entry)
            return;
         while(entry->next == NULL) {
            cpu_relax();
         }
         entry->next.load()->state = 0;
      }

      explicit qspinlock(void): tail(NULL) {}
};

}

#endif
