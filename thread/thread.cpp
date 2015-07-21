#include <iostream>
#include <climits>
#include <thread>
#include <unistd.h>

#include "thread/thread.hpp"
#include "db/database.hpp"
#include "vm/state.hpp"
#include "interface.hpp"
#include "vm/priority.hpp"
#include "vm/exec.hpp"
#include "machine.hpp"

using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

//#define DEBUG_QUEUE

#ifdef PROFILE_QUEUE
static atomic<size_t> prio_count(0);
static atomic<size_t> prio_immediate(0);
static atomic<size_t> prio_marked(0);
static atomic<size_t> prio_moved_pqueue(0);
static atomic<size_t> prio_add_pqueue(0);
static atomic<size_t> prio_removed_pqueue(0);
static atomic<size_t> prio_nodes_compared(0);
static atomic<size_t> prio_nodes_changed(0);
#endif

extern void execute_const_code();

namespace sched {

tree_barrier *thread::thread_barrier(nullptr);
termination_barrier *thread::term_barrier(nullptr);

std::atomic<bool> thread::stop_flag(false);

static std::mutex m;

void thread::do_loop(void) {
   db::node *node(nullptr);
#if 0
   int count{0};
   execution_time t;
#endif

   while (true) {
      node = get_work();
      if (node == nullptr) break;
#if 0
      count++;
      m.lock();
      cout << get_id() << " run " << node->get_id() << "(" << count << "/2)\n";
      m.unlock();
#endif
      assert(node != nullptr);
#if 0
      t.start();
#endif
      state.run_node(node);
#if 0
      t.stop();
#endif
      if (stop_flag) {
         killed_while_active();
         return;
      }
   }
   if (stop_flag) {
      killed_while_active();
      return;
   }
#if 0
   m.lock();
   cout << get_id() << " run " << t << "\n";
   m.unlock();
#endif
}

void thread::loop(void) {
   init(All->NUM_THREADS);

   do_loop();

   assert_end();
   end();
}

void thread::init_barriers(const size_t num_threads) {
   thread_barrier = new tree_barrier(num_threads);
   term_barrier = new termination_barrier(num_threads);
}

void thread::assert_end(void) const {
   assert(is_inactive());
   assert(all_threads_finished());
}

void thread::new_thread_work(thread *to, vm::tuple *tpl,
                             const vm::predicate *pred) {
   db::node *n(to->thread_node);

   LOCK_STACK(nodelock);
   NODE_LOCK(n, nodelock, node_lock);

   n->add_work_others(tpl, pred, vm::POSITIVE_DERIVATION, 0);
#ifdef INSTRUMENTATION
   sent_facts_other_thread++;
   all_transactions++;
   thread_transactions++;
#endif
   comm_threads.set_bit(to->get_id());
   NODE_UNLOCK(n, nodelock);
}

void thread::new_work(node *from, node *to, vm::tuple *tpl, vm::predicate *pred,
                      const derivation_direction dir, const depth_t depth) {
   assert(is_active());
   (void)from;

   LOCK_STACK(nodelock);
   NODE_LOCK(to, nodelock, node_lock);

   thread *owner(to->get_owner());
   if (owner == this) {
      {
         MUTEX_LOCK_GUARD(to->database_lock, database_lock);
         to->add_work_myself(tpl, pred, dir, depth);
      }
#ifdef INSTRUMENTATION
      sent_facts_same_thread++;
      all_transactions++;
#endif
      if (!to->active_node()) add_to_queue(to);
   } else {
      LOCK_STACK(databaselock);

      if (to->database_lock.try_lock1(LOCK_STACK_USE(databaselock))) {
         LOCKING_STAT(database_lock_ok);
         to->add_work_myself(tpl, pred, dir, depth);
#ifdef INSTRUMENTATION
         sent_facts_other_thread_now++;
         all_transactions++;
         thread_transactions++;
#endif
         MUTEX_UNLOCK(to->database_lock, databaselock);
      } else {
         LOCKING_STAT(database_lock_fail);
         to->add_work_others(tpl, pred, dir, depth);
#ifdef INSTRUMENTATION
         sent_facts_other_thread++;
         all_transactions++;
         thread_transactions++;
#endif
      }
      if (!to->active_node()) {
         owner->add_to_queue(to);
         comm_threads.set_bit(owner->get_id());
      }
   }

   NODE_UNLOCK(to, nodelock);
}

#ifdef TASK_STEALING
bool thread::go_steal_nodes(void) {
   // Function returns 'true' if there are new nodes in the thread.
   // New nodes come either from new work or stolen nodes.
   // When returning true, the thread is in the 'active' state
   // while when returning false, it must be inactive.
   if (All->NUM_THREADS == 1) return false;

   ins_sched;
#ifdef STEAL_ONE
#define NODE_BUFFER_SIZE 1
#elif defined(STEAL_HALF)
#define NODE_BUFFER_SIZE 16
#endif
   db::node *node_buffer[NODE_BUFFER_SIZE];
   bool activated{false};
   bool has_work{false};

   for (size_t i(0); i < All->NUM_THREADS; ++i) {
      const size_t tid((next_thread + i) % All->NUM_THREADS);
      if (this == All->SCHEDS[tid]) continue;

      assert(tid < All->NUM_THREADS);
      thread *target((thread *)All->SCHEDS[tid]);

      if (!target->is_active() || !target->has_work()) continue;

      if(!activated) {
            has_work |= set_active_if_inactive();
            activated = true;
      }
      const size_t stolen(target->steal_nodes(node_buffer, NODE_BUFFER_SIZE));

      if (stolen == 0) continue;

      has_work |= true;
#ifdef INSTRUMENTATION
      stolen_total += stolen;
#endif

      for (size_t i(0); i < stolen; ++i) {
         db::node *node(node_buffer[i]);

         LOCK_STACK(nodelock);
         NODE_LOCK(node, nodelock, node_lock);
         if (node->node_state() != STATE_STEALING) {
            // node was put in the queue again, give up.
            NODE_UNLOCK(node, nodelock);
            continue;
         }
         if (node->is_static()) {
            if (node->get_owner() != target) {
               // set-affinity was used and the node was changed to another
               // scheduler
               move_node_to_new_owner(node, node->get_owner());
               NODE_UNLOCK(node, nodelock);
               continue;
            } else {
               // meanwhile the node is now set as static.
               // set ourselves as the static scheduler
               make_node_static(node, this);
            }
         }
         // XXX add to queue at once.
         node->set_owner(this);
         add_to_queue(node);
         NODE_UNLOCK(node, nodelock);
      }
      // set the next thread to the current one
      next_thread = tid;
      if(!activated)
         set_active_if_inactive();
      return true;
   }

   if(activated) {
      if(has_work)
         return true;
      set_inactive_if_no_work();
      return false;
   } else
      return false;
}

size_t thread::steal_nodes(db::node **buffer, const size_t max) {
   steal_flag = !steal_flag;
   size_t stolen = 0;

#ifdef STEAL_ONE
   if (max == 0) return 0;

   db::node *node(nullptr);
   if (steal_flag) {
      if (!queues.moving.empty()) {
         if (queues.moving.pop_tail(node, STATE_STEALING)) {
            buffer[stolen++] = node;
         }
      } else if (!prios.moving.empty()) {
         node = prios.moving.pop(STATE_STEALING);
         if (node) buffer[stolen++] = node;
      }
   } else {
      if (!prios.moving.empty()) {
         node = prios.moving.pop(STATE_STEALING);
         if (node) buffer[stolen++] = node;
      } else if (!queues.moving.empty()) {
         if (queues.moving.pop_head(node, STATE_STEALING))
            buffer[stolen++] = node;
      }
   }
#elif defined(STEAL_HALF)
   if (steal_flag) {
      if (!queues.moving.empty())
         stolen = queues.moving.pop_tail_half(buffer, max, STATE_STEALING);
      else if (!prios.moving.empty())
         stolen = prios.moving.pop_half(buffer, max, STATE_STEALING);
   } else {
      if (!prios.moving.empty())
         stolen = prios.moving.pop_half(buffer, max, STATE_STEALING);
      else if (!queues.moving.empty())
         stolen = queues.moving.pop_tail_half(buffer, max, STATE_STEALING);
   }
#else
#error "Must select a way to steal nodes."
#endif
   return stolen;
}
#endif

void thread::killed_while_active(void) { set_force_inactive(); }

bool thread::busy_wait(void) {
#ifdef TASK_STEALING
   if (!theProgram->is_static_priority()) {
      if (work_stealing && go_steal_nodes()) {
         ins_active;
         return true;
      }
   }
#endif

#ifdef TASK_STEALING
   size_t count(0);
#endif

   while (!has_work()) {
#ifdef TASK_STEALING
#define STEALING_ROUND_MIN 1024
#define STEALING_ROUND_MAX (4 * 4096)
      if (!theProgram->is_static_priority() && work_stealing) {
         count++;
         if (count == backoff) {
            count = 0;
            if (go_steal_nodes()) {
               ins_active;
               backoff = max(backoff >> 1, (size_t)STEALING_ROUND_MIN);
               return true;
            } else {
               backoff = min((size_t)STEALING_ROUND_MAX, backoff << 1);
            }
         }
      }
#endif
      const bool has_new_work(set_inactive_if_no_work());
      if (has_new_work) {
         ins_active;
         return true;
      } else {
         ins_idle;
      }
      if (all_threads_finished() || stop_flag) {
         assert(is_inactive());
         return false;
      }
      cpu_relax();
      std::this_thread::yield();
   }

   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   ins_active;
   assert(is_active());

   return true;
}

void thread::make_current_node_inactive(void) {
   current_node->make_inactive();
   current_node->remove_temporary_priority();
}

inline bool thread::check_if_current_useless(void) {
   LOCK_STACK(curlock);
   if (current_node->has_new_owner()) {
      NODE_LOCK(current_node, curlock, node_lock);
      // the node has changed to another scheduler!
      assert(current_node->get_static() != this);
      current_node->set_owner(current_node->get_static());
      current_node->remove_temporary_priority();
      // move to new node
      move_node_to_new_owner(current_node, current_node->get_owner());
      NODE_UNLOCK(current_node, curlock);
      current_node = nullptr;
      return true;
   } else if (!current_node->unprocessed_facts) {
      NODE_LOCK(current_node, curlock, node_lock);

      if (!current_node->unprocessed_facts) {
         make_current_node_inactive();
#ifdef GC_NODES
         if (current_node->garbage_collect())
            delete_node(current_node);
         else
            NODE_UNLOCK(current_node, curlock);
#else
         NODE_UNLOCK(current_node, curlock);
#endif
         current_node = nullptr;
         return true;
      }
      NODE_UNLOCK(current_node, curlock);
   }

   assert(current_node->unprocessed_facts);
   return false;
}

inline bool thread::set_next_node(void) {
#ifndef DIRECT_PRIORITIES
   check_priority_buffer();
#endif

   if (current_node != nullptr) check_if_current_useless();

   while (current_node == nullptr) {
      current_node = prios.moving.pop_best(prios.stati, STATE_WORKING);
      if (current_node) {
#ifdef DEBUG_QUEUE
         cout << "Got node " << current_node->get_id() << " with prio "
              << current_node->get_priority() << endl;
#endif
         break;
      }

      if (pop_node_from_queues()) {
#ifdef DEBUG_QUEUE
         cout << "Got node " << current_node->get_id() << endl;
#endif
         break;
      }

      // process the thread node because it may have facts.
      if (thread_node && thread_node->unprocessed_facts) {
#ifdef DEBUG_QUEUE
         cout << "Running thread node\n";
#endif
         current_node = thread_node;
         break;
      }

      if (!has_work()) {
         for (auto it(comm_threads.begin(All->NUM_THREADS)); !it.end(); ++it) {
            const size_t id(*it);
            thread *target(static_cast<thread *>(All->SCHEDS[id]));
            target->activate_thread();
         }
         comm_threads.clear(All->NUM_THREADS_NEXT_UINT);
         if (!busy_wait()) return false;
      }
   }

   ins_active;

   assert(current_node != nullptr);

   return true;
}

node *thread::get_work(void) {
   if (!set_next_node()) return nullptr;

   ins_active;
   assert(current_node != nullptr);

#ifdef INSTRUMENTATION
   node_difference += current_node->get_translated_id() - last_node;
   last_node = current_node->get_translated_id();
#endif

   return current_node;
}

void thread::end(void) {
#if defined(DEBUG_PRIORITIES) && defined(PROFILE_QUEUE)
   cout << "prio_immediate: " << prio_immediate << endl;
   cout << "prio_marked: " << prio_marked << endl;
   cout << "prio_count: " << prio_count << endl;
#endif

#if defined(DEBUG_PRIORITIES)
   size_t total_prioritized(0);
   size_t total_nonprioritized(0);

   database::map_nodes::iterator it(
       state::DATABASE->get_node_iterator(All->MACHINE->find_first_node(id)));
   database::map_nodes::iterator end(
       state::DATABASE->get_node_iterator(All->MACHINE->find_last_node(id)));

   for (; it != end; ++it) {
      db::node *cur_node(it->second);

      if (cur_node->has_been_prioritized)
         ++total_prioritized;
      else
         ++total_nonprioritized;
   }

   cout << "Number of prioritized nodes: " << total_prioritized << endl;
   cout << "Number of non prioritized nodes: " << total_nonprioritized << endl;
#endif
}

#ifndef COMPILED
void thread::run_const_code(void) {
   theProgram->fix_const_references();

   // no node or tuple whatsoever
   state.setup(nullptr, POSITIVE_DERIVATION, 0);

   execute_process(theProgram->get_const_bytecode(), state, nullptr, nullptr);
}
#endif

void thread::init(const size_t) {
   // normal priorities
   if (theProgram->is_priority_desc()) {
      prios.moving.set_type(HEAP_DESC);
      prios.stati.set_type(HEAP_DESC);
   } else {
      prios.moving.set_type(HEAP_ASC);
      prios.stati.set_type(HEAP_ASC);
   }

   auto it(All->DATABASE->get_node_iterator(All->MACHINE->find_first_node(id)));
   auto end(All->DATABASE->get_node_iterator(All->MACHINE->find_last_node(id)));
   priority_t initial(theProgram->get_initial_priority());

   if (initial == vm::no_priority_value()) {
      for (; it != end; ++it) {
         db::node *cur_node(init_node(it));
         queues.moving.push_tail(cur_node);
      }
   } else {
      prios.moving.start_initial_insert(All->MACHINE->find_owned_nodes(id));
      size_t total{0};

      for (size_t i(0); it != end; ++it, ++i) {
         db::node *cur_node(init_node(it));

         prios.moving.initial_fast_insert(cur_node, initial, i);
         total++;
      }
      //cout << total << endl;
   }

   threads_synchronize();
   bool sync{false};
#ifdef COMPILED
   if (theProgram->has_const_code()) {
      sync = true;
      if (leader_thread()) execute_const_code();
   }
#else
   if (leader_thread()) theProgram->cleanup_node_references();
   if (theProgram->has_const_code()) {
      sync = true;
      if (leader_thread()) run_const_code();
   }
#endif
   if (theProgram->has_thread_predicates()) {
      thread_node = node_handler.create_node();
      setup_thread_node();
      sync = true;
   }

   if (sync) threads_synchronize();
}

void thread::setup_thread_node() {
   thread_node->set_owner(this);

   vm::predicate *init_thread_pred(vm::theProgram->get_init_thread_predicate());
   vm::tuple *init_tuple(
       vm::tuple::create(init_thread_pred, &(thread_node->alloc)));
   thread_node->add_linear_fact(init_tuple, init_thread_pred);

   if (vm::theProgram->has_special_fact(vm::special_facts::THREAD_LIST)) {
      vm::predicate *tlist_pred(
          vm::theProgram->get_special_fact(vm::special_facts::THREAD_LIST));
      db::array *s(thread_node->pers_store.get_array(tlist_pred));
      s->init(1, tlist_pred, &(thread_node->alloc));
      vm::tuple *tpl(s->add_next(tlist_pred));
      runtime::cons *ls(runtime::cons::null_list());
      for (size_t i(vm::All->NUM_THREADS); i > 0; --i) {
         thread *x(vm::All->SCHEDS[i - 1]);
         vm::tuple_field f;
         f.ptr_field = (vm::ptr_val)x;
         assert(x);
         ls = runtime::cons::create(ls, f, vm::TYPE_THREAD);
      }
      tpl->set_cons(0, ls);
      thread_node->matcher.new_persistent_fact(tlist_pred->get_id());
   }

   if (vm::theProgram->has_special_fact(vm::special_facts::OTHER_THREAD)) {
      vm::predicate *other_pred(
          vm::theProgram->get_special_fact(vm::special_facts::OTHER_THREAD));
      db::array *s(thread_node->pers_store.get_array(other_pred));
      s->init(vm::All->NUM_THREADS-1, other_pred, &(thread_node->alloc));
      for (size_t i(0); i < vm::All->NUM_THREADS; ++i) {
         if (this == All->SCHEDS[i]) continue;
         vm::tuple *tpl(s->add_next(other_pred));
         tpl->set_thread(0, (vm::thread_val)All->SCHEDS[i]);
         tpl->set_int(1, i);
         thread_node->matcher.new_persistent_fact(other_pred->get_id());
      }
   }

   if (vm::theProgram->has_special_fact(vm::special_facts::LEADER_THREAD)) {
      vm::predicate *leader_thread(
          vm::theProgram->get_special_fact(vm::special_facts::LEADER_THREAD));
      db::array *s(thread_node->pers_store.get_array(leader_thread));
      s->init(1, leader_thread, &(thread_node->alloc));
      vm::tuple *tpl(s->add_next(leader_thread));
      tpl->set_thread(0, (vm::thread_val)All->SCHEDS[0]);
      thread_node->matcher.new_persistent_fact(leader_thread->get_id());
   }

   thread_node->unprocessed_facts = true;
}

#ifdef INSTRUMENTATION
void thread::write_slice(statistics::slice &sl) {
   sl.state = ins_state;
   sl.consumed_facts = state.instr_facts_consumed.exchange(0);
   sl.derived_facts = state.instr_facts_derived.exchange(0);
   sl.rules_run = state.instr_rules_run.exchange(0);

   sl.work_queue = queue_size();
   sl.sent_facts_same_thread = sent_facts_same_thread.exchange(0);
   sl.sent_facts_other_thread = sent_facts_other_thread.exchange(0);
   sl.sent_facts_other_thread_now = sent_facts_other_thread_now.exchange(0);
   sl.priority_nodes_thread = priority_nodes_thread.exchange(0);
   sl.priority_nodes_others = priority_nodes_others.exchange(0);
   if (All->THREAD_POOLS[get_id()])
      sl.bytes_used = All->THREAD_POOLS[get_id()]->bytes_in_use;
   else
      sl.bytes_used = 0;
   sl.node_lock_ok = node_lock_ok.exchange(0);
   sl.node_lock_fail = node_lock_fail.exchange(0);
   sl.thread_transactions = thread_transactions.exchange(0);
   sl.all_transactions = all_transactions.exchange(0);
   sl.node_difference = node_difference;

#ifdef TASK_STEALING
   sl.stolen_nodes = stolen_total.exchange(0);
#endif
}
#endif

thread::thread(const vm::process_id _id)
    : id(_id),
    state(this)
#ifdef TASK_STEALING
      ,
      rand(_id * 1000),
      next_thread(rand(All->NUM_THREADS)),
      backoff(STEALING_ROUND_MIN)
#endif
#ifndef DIRECT_PRIORITIES
      ,
      priority_buffer(
          std::min(PRIORITY_BUFFER_SIZE,
                   vm::All->DATABASE->num_nodes() / vm::All->NUM_THREADS))
#endif
{
   bitmap::create(comm_threads, All->NUM_THREADS_NEXT_UINT);
#if 0
   state = mem::allocator<vm::state>().allocate(1);
   mem::allocator<vm::state>().construct(state, this);
#endif
}

thread::~thread(void) {
   bitmap::destroy(comm_threads, All->NUM_THREADS_NEXT_UINT);
   assert(tstate == THREAD_INACTIVE);
   if (theProgram->has_thread_predicates()) delete_node(thread_node);
}
}
