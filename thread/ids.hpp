
#ifndef THREAD_IDS_HPP
#define THREAD_IDS_HPP

#include <unordered_map>
#include <unordered_set>

#include "db/database.hpp"
#include "db/node.hpp"

#define ALLOCATED_IDS (16*1024)

namespace sched
{

class ids
{
   // class to hold requested node ID's and ID's that need to be deleted.
   private:

      db::node::node_id next_available_id;
      db::node::node_id end_available_id;

      db::node::node_id next_translated_id;

      db::node *allocated_nodes{nullptr};
      db::node *freed_nodes{nullptr};
      size_t total_allocated{0};
      size_t total_freed{0};
      std::atomic<size_t> deleted_by_others{0};

      size_t next_allocation;

      db::node *remove_node_from_allocated(db::node *);
      void add_allocated_node(db::node *);
      void free_destroyed_nodes();
      void allocate_more_ids();

   public:

      void commit(void);

      db::node *create_node(void);

      void delete_node(db::node *);

      explicit ids(void);
};

}

#endif
