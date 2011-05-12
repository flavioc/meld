
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
   

public:
   
   void token_terminate_iteration(void);
   bool busy_loop_token(const bool);
   
   void token_is_over(void) { tok.set_white(); global_tok.set_white(); }
   void token_is_not_over(void) { tok.set_black(); }
   
   void one_message_received(void);
   inline void transmitted(const size_t total) { tok.transmitted(total); }
   inline void try_fetch_token_as_worker_if_global(void) {
      if(!has_global_tok)
         try_fetch_token_as_worker();
   }
   
   explicit tokenizer(void);
   
   virtual ~tokenizer(void);
};

}

#endif
