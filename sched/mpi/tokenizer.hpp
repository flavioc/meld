
#ifndef SCHED_MPI_TOKENIZER_HPP
#define SCHED_MPI_TOKENIZER_HPP

#include "sched/mpi/token.hpp"

#ifdef COMPILE_MPI

namespace sched
{

class tokenizer
{
protected:
   
   token tok;
   bool has_global_tok;
   token global_tok;
   
   void try_fetch_token_as_worker(vm::all *);
   void send_token_as_leader(vm::all *);
   void send_token_as_idler(vm::all *);
   bool try_fetch_token_as_leader(vm::all *);
   bool try_fetch_token_as_idler(vm::all *);
   bool try_fetch_end_iteration(vm::all *);
   void do_collective_end_iteration(size_t, vm::all *);
   
public:
   
   void token_terminate_iteration(void);
   bool busy_loop_token(const bool, vm::all *);
   
   void messages_received(const size_t);
   inline void messages_transmitted(const size_t total) { tok.transmitted(total); }
   
   // init function to be called after MPI subsystem is set up properly
   void init(void);
   
   // clean up function
   void end(void);
   
   explicit tokenizer(void);
   
   virtual ~tokenizer(void);
};

}
#endif

#endif
