
#include <iostream>
#include <tr1/unordered_set>

#include "mem/thread.hpp"

using namespace boost;
using namespace std;
using namespace std::tr1;

#ifdef ALLOCATOR_ASSERT
mutex allocator_mtx;
unordered_set<void*> mem_set;
#endif

namespace mem
{
   
static bool init(void);

static pthread_key_t pool_key;
static bool started(init());

static void
cleanup_memsystem(void)
{
   pthread_key_delete(pool_key);
}

static bool
init(void)
{
   int ret(pthread_key_create(&pool_key, NULL));
   assert(ret == 0);
   atexit(cleanup_memsystem);
   return true;
}

void
create_pool(void)
{
   thread::id pid(this_thread::get_id());
   
   pool *pl(new pool());
   pthread_setspecific(pool_key, pl);
}

void
delete_pool(void)
{
   pool *pl((pool*)pthread_getspecific(pool_key));
   
   if(pl != NULL)
      delete pl;
}

void
ensure_pool(void)
{
   if(NULL == get_pool())
      create_pool();
}

pool*
get_pool(void)
{
   pool *pl((pool*)pthread_getspecific(pool_key));
   
   if(pl == NULL) {
      create_pool();
      return get_pool();
   }
   
   return pl;
}

void
cleanup(const size_t num_threads)
{
   (void)num_threads;

   pthread_setspecific(pool_key, NULL);

#ifdef ALLOCATOR_ASSERT
   assert(mem_set.size() == 0);
#endif
}

}
