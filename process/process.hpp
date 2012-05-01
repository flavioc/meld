
#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <list>
#include <stdexcept>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "conf.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"
#include "db/node.hpp"
#include "db/database.hpp"
#include "sched/base.hpp"

namespace process
{

class process
{
private:
   
   const vm::process_id id;
   boost::thread *thread;
   sched::base *scheduler;
   
   vm::state state;
   
   void do_work(work&);
   void do_tuple_action(db::node *, vm::tuple *, const vm::ref_count);
   void do_tuple_add(db::node *, vm::tuple *, const vm::ref_count);
   void do_agg_tuple_add(db::node *, vm::tuple *, const vm::ref_count);
   void do_loop(void);
   void loop(void);

public:
   
   inline size_t num_iterations(void) const { return get_scheduler()->num_iterations(); }
   
   void start(void);
      
   inline void join(void) { thread->join(); }
   
   inline const sched::base *get_scheduler(void) const { return scheduler; }
   inline sched::base *get_scheduler(void) { return scheduler; }
   
   explicit process(const vm::process_id, sched::base *);
   
   ~process(void);
};

class process_exec_error : public std::runtime_error {
 public:
    explicit process_exec_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};

}

#endif
