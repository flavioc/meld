
#include "process/remote.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"
#include "sched/mpi/tokenizer.hpp"
#include "utils/utils.hpp"

using namespace vm;
using namespace process;
using namespace utils;
using namespace std;

#ifdef COMPILE_MPI

namespace sched
{
   
void
tokenizer::messages_received(const size_t total)
{
   tok.received(total);
   tok.set_black();
}

void
tokenizer::try_fetch_token_as_worker(vm::all *all)
{
   assert(has_global_tok == false);

   if(all->ROUTER->receive_token(global_tok)) {
      has_global_tok = true;
   }
}

void
tokenizer::send_token_as_leader(vm::all *all)
{
   assert(remote::self->is_leader());
   assert(has_global_tok == true);
   
#ifdef DEBUG_REMOTE
   cout << "Sent a white token / 0 from leader to" << endl;
#endif

   tok.set_white();
   global_tok.set_white();
   global_tok.reset_count();
   has_global_tok = false;
   
   assert(global_tok.is_white());
   assert(global_tok.is_zero());
   all->ROUTER->send_token(global_tok);
   
   assert(tok.is_white());
}

bool
tokenizer::try_fetch_token_as_leader(vm::all *all)
{
   assert(remote::self->is_leader());
   assert(has_global_tok == false);
   
   if(all->ROUTER->receive_token(global_tok)) {
      has_global_tok = true;
      global_tok.add_count(tok.get_count());
#ifdef DEBUG_SAFRAS
      cout << "Add count " << tok.get_count() << endl;
#endif
      if(global_tok.is_white() && global_tok.is_zero() && tok.is_white()) {
#ifdef DEBUG_SAFRAS
         cout << "TERMINATE!" << endl;
#endif
         assert(global_tok.is_white());
         assert(global_tok.is_zero());
         assert(tok.is_white());
         
         do_collective_end_iteration(1, all);
         
         return true;
      } else
         send_token_as_leader(all);
   }
   
   return false;
}

void
tokenizer::send_token_as_idler(vm::all *all)
{
   assert(!remote::self->is_leader());
   assert(has_global_tok == true);
   
#ifdef DEBUG_SAFRAS
   cout << "Send token as idler in black" << endl;
#endif

   global_tok.add_count(tok.get_count());
   global_tok.set_black();
   
   assert(global_tok.is_black());
   
   all->ROUTER->send_token(global_tok);
   has_global_tok = false;
}

bool
tokenizer::try_fetch_token_as_idler(vm::all *all)
{
   assert(!remote::self->is_leader());
   assert(has_global_tok == false);
   
   if(all->ROUTER->receive_token(global_tok)) {
      has_global_tok = true;
      global_tok.add_count(tok.get_count());
#ifdef DEBUG_SAFRAS
      cout << "Received new tok and adding count " << tok.get_count() << endl;
#endif

      if(tok.is_black())
         global_tok.set_black();
      
      tok.set_white();
#ifdef DEBUG_SAFRAS
      if(global_tok.is_black())
         cout << "Resending as black\n";
      else
         cout << "Resending as white\n";
#endif

      assert(has_global_tok == true);
      all->ROUTER->send_token(global_tok);
      has_global_tok = false;
      
      assert(tok.is_white());
      
      return true;
   }
   
   assert(has_global_tok == false);
   
   return false;
}

void
tokenizer::do_collective_end_iteration(size_t stage, vm::all *all)
{
   for( ; stage <= upper_log2(remote::world_size); ++stage) {
      if(remote::self->get_rank() < power<remote::remote_id>(2, stage - 1)) {
         const remote::remote_id dest(remote::self->get_rank() + power<remote::remote_id>(2, stage - 1));
         
         assert(dest >= 0);
         
         if((size_t)dest < remote::world_size)
            all->ROUTER->send_end_iteration(stage, dest);
      } else if(remote::self->get_rank() >= power<remote::remote_id>(2, stage - 1)
                  && remote::self->get_rank() < power<remote::remote_id>(2, stage))
      {
         const remote::remote_id source(remote::self->get_rank() - power<remote::remote_id>(2, stage - 1));
         all->ROUTER->receive_end_iteration(source);
      }
   }
}

static inline remote::remote_id
find_source_collective(void)
{
   for(size_t stage(1); stage <= upper_log2(remote::world_size); ++stage) {
      if(remote::self->get_rank() < power<remote::remote_id>(2, stage - 1))
         assert(false);
      else if(remote::self->get_rank() >= power<remote::remote_id>(2, stage - 1)
               && remote::self->get_rank() < power<remote::remote_id>(2, stage))
         return remote::self->get_rank() - power<remote::remote_id>(2, stage - 1);
   }
   
   assert(0);
   
   return -1;
}

bool
tokenizer::try_fetch_end_iteration(vm::all *all)
{
   size_t stage;
   
   if(all->ROUTER->received_end_iteration(stage, find_source_collective())) {
      do_collective_end_iteration(stage + 1, all);
      return true;
   }
   return false;
}

bool
tokenizer::busy_loop_token(const bool turned_inactive, vm::all *all)
{
   if(turned_inactive) {
      if(remote::self->is_leader()) {
         if(has_global_tok && !all->ROUTER->use_mpi())
            return false;
         if(has_global_tok)
            send_token_as_leader(all);
         else {
            if(try_fetch_token_as_leader(all))
               return false;
         }
      } else {
         if(has_global_tok)
            send_token_as_idler(all);
         else {
            if(!try_fetch_token_as_idler(all)) {
               if(try_fetch_end_iteration(all))
                  return false;
            }
         }
      }
   } else {
      if(!has_global_tok)
         try_fetch_token_as_worker(all);
   }
   
   return true;
}

void
tokenizer::token_terminate_iteration(void)
{
   if(remote::self->is_leader()) {
      assert(global_tok.is_zero());
      assert(global_tok.is_white());
      assert(has_global_tok == true);   
   }
   
   assert(tok.is_white());
   
   tok.reset_count();
}
   
tokenizer::tokenizer(void)
{
}

tokenizer::~tokenizer(void)
{
}

void
tokenizer::init(void)
{
   if(remote::self->is_leader()) {
      has_global_tok = true;
      global_tok.set_white();
   } else {
      has_global_tok = false;
   }
   tok.set_white();
}

void
tokenizer::end(void)
{
   assert(tok.is_white());
}

}
#endif
