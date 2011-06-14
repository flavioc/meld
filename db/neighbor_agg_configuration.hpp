
#ifndef DB_NEIGHBOR_AGG_CONFIGURATION_HPP
#define DB_NEIGHBOR_AGG_CONFIGURATION_HPP

#include "db/edge_set.hpp"
#include "db/agg_configuration.hpp"

namespace db
{

class neighbor_agg_configuration: public agg_configuration
{
private:
   
   edge_set sent;
   
public:
   
   // XXX need to find a better way to change the class allocator using inheritance
   inline void* operator new(size_t size)
   {
      return mem::allocator<neighbor_agg_configuration>().allocate(1);
   }
   
   static inline void operator delete(void *ptr)
   {
      mem::allocator<neighbor_agg_configuration>().deallocate((neighbor_agg_configuration*)ptr, 1);
   }
   
   virtual void add_to_set(vm::tuple *, const vm::ref_count);
   
   bool all_present(const edge_set&) const;
   
   explicit neighbor_agg_configuration(const vm::predicate *_pred):
      agg_configuration(_pred)
   {
   }

   virtual ~neighbor_agg_configuration(void) {}
};

}

#endif
