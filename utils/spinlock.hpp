
#ifndef UTILS_SPINLOCK_HPP
#define UTILS_SPINLOCK_HPP

namespace utils {

typedef union ticketlock ticketlock;

union ticketlock
{
   unsigned u;
   struct
   {
      unsigned short ticket;
      unsigned short users;
   } s;
};

class spinlock
{
   private:
      ticketlock t;

   public:
      /* Compile read-write barrier */
#define barrier() asm volatile("": : :"memory")

      /* Pause instruction to prevent excess processor bus usage */ 
#define cpu_relax() asm volatile("pause\n": : :"memory")
#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))

      inline bool try_lock(void)
      {
         unsigned short me = t.s.users;
         unsigned short menew = me + 1;
         unsigned cmp = ((unsigned) me << 16) + me;
         unsigned cmpnew = ((unsigned) menew << 16) + me;

         if (cmpxchg(&t.u, cmp, cmpnew) == cmp) return true;

         return false;
      }

      inline void unlock(void)
      {
         barrier();
         t.s.ticket++;
      }

      inline void lock(void)
      {
         unsigned short me = atomic_xadd(&t.s.users, 1);

         while (t.s.ticket != me) cpu_relax();
      }

      explicit spinlock(void) {
         t.u = 0;
      }
};

}

#endif
