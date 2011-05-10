
#ifndef SCHED_TOKENIZER_HPP
#define SCHED_TOKENIZER_HPP

#include "sched/token.hpp"

namespace sched
{

class tokenizer
{
protected:
   
   token tok;
   bool has_global_tok;
   token global_tok;
   
   void try_fetch_token_as_worker(void);
   void send_token_as_leader(void);
   void send_token_as_idler(void);
   bool try_fetch_token_as_leader(void);
   bool try_fetch_token_as_idler(void);
   bool try_fetch_end_iteration(void);
   void do_collective_end_iteration(size_t);
   bool busy_loop_token(const bool);
   void token_terminate_iteration(void);

public:
   
   explicit tokenizer(void);
   
   virtual ~tokenizer(void);
};

}

#endif
