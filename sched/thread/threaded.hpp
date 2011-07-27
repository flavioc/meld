
#ifndef SCHED_THREAD_THREADED_HPP
#define SCHED_THREAD_THREADED_HPP

#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
#include <vector>

#include "vm/defs.hpp"
#include "utils/types.hpp"
#include "utils/spinlock.hpp"
#include "utils/atomic.hpp"
#include "sched/base.hpp"
#include "sched/thread/termination_barrier.hpp"
#include "sched/thread/state.hpp"
#include "utils/tree_barrier.hpp"

namespace sched
{

class threaded
{
private:
   
   static termination_barrier *term_barrier;
     
   volatile thread_state state;
   
   DEFINE_PADDING;
   
protected:
   
   static utils::tree_barrier *thread_barrier;
   
   DEFINE_PADDING;
   
	utils::spinlock lock;
	
   static volatile size_t round_state;
   size_t thread_round_state;
   static utils::atomic<size_t> total_in_agg;
   
   static std::vector<sched::base*> ALL_THREADS;
   
   static inline void add_thread(sched::base *add)
   {
      ALL_THREADS[add->get_id()] = add;
   }
   
#define threads_synchronize() thread_barrier->wait(get_id())
   
   static void init_barriers(const size_t);
   
   inline void set_active(void)
   {
      assert(state == THREAD_INACTIVE);
      state = THREAD_ACTIVE;
      term_barrier->is_active();
   }
   
   inline void set_inactive(void)
   {
      assert(state == THREAD_ACTIVE);
      state = THREAD_INACTIVE;
      term_barrier->is_inactive();
   }
   
   inline void set_active_if_inactive(void)
   {
      if(is_inactive()) {
			utils::spinlock::scoped_lock l(lock);
         if(is_inactive())
            set_active();
      }
   }
   
   static inline size_t num_active(void) { return term_barrier->num_active(); }
   
   inline bool is_inactive(void) const { return state == THREAD_INACTIVE; }
   inline bool is_active(void) const { return state == THREAD_ACTIVE; }
   
   static inline void reset_barrier(void) { term_barrier->reset(); }
   
   inline bool all_threads_finished(void) const
   {
      return term_barrier->all_finished();
   }
   
public:
   
   explicit threaded(void): state(THREAD_ACTIVE),
      thread_round_state(1)
   {
   }
   
   virtual ~threaded(void)
   {
      assert(state == THREAD_INACTIVE);
   }
};

#define DEFINE_START_FUNCTION(CLASS)                  \
   static std::vector<sched::base*>&                  \
   start(const size_t num_threads)                    \
   {                                                  \
      init_barriers(num_threads);                     \
      for(vm::process_id i(0); i < num_threads; ++i)  \
         add_thread(new CLASS(i));                    \
      assert_thread_disable_work_count();             \
      return ALL_THREADS;                             \
   }

#define BUSY_LOOP_MAKE_INACTIVE()         \
   if(is_active() && !has_work()) {       \
      spinlock::scoped_lock l(lock);      \
      if(!has_work()) {                   \
         if(is_active())                  \
            set_inactive();               \
      } else                              \
         break;                           \
   }
   
#define BUSY_LOOP_CHECK_TERMINATION_THREADS()   \
   if(!has_work() && is_inactive() && all_threads_finished()) {      \
      assert(!has_work());                                           \
      assert(is_inactive());                                         \
      assert(all_threads_finished());                                \
      return false;                                                  \
   }
   
#define MAKE_OTHER_ACTIVE(OTHER) \
   if((OTHER)->is_inactive() && (OTHER)->has_work()) {   \
      spinlock::scoped_lock l(OTHER->lock);              \
      if((OTHER)->is_inactive() && (OTHER)->has_work())  \
         (OTHER)->set_active();                          \
   }

}

#endif
