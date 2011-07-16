
#ifndef VM_STRATA_HPP
#define VM_STRATA_HPP

#include <vector>
#include <iostream>

#include "conf.hpp"
#include "vm/predicate.hpp"

namespace vm
{

class strata
{
private:
   
   std::vector<size_t> vec;
   size_t current_level;

public:
   
   inline size_t get_current_level(void)
   {
      if(vec[current_level] != 0)
         return current_level;
      
      size_t cur(current_level);
      
      while(current_level < predicate::MAX_STRAT_LEVEL && vec[cur] == 0) {
         cur++;
      }
      
      current_level = cur;
      return current_level;
   }
   
   inline void push_queue(const size_t level)
   {
      vec[level]++;
   }
   
   inline void pop_queue(const size_t level)
   {
      assert(vec[level] > 0);
      vec[level]--;
   }
   
   explicit strata(void):
      vec(predicate::MAX_STRAT_LEVEL),
      current_level(0)
   {
      vec.resize(predicate::MAX_STRAT_LEVEL);
#ifndef NDEBUG
      assert(vec.size() == predicate::MAX_STRAT_LEVEL);
      for(size_t i(0); i < vec.size(); ++i) {
         assert(vec[i] == 0);
      }
#endif
   }
   
   ~strata(void) {}  
};

}

#endif
