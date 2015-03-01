
#ifndef THREAD_IDS_HPP
#define THREAD_IDS_HPP

#include <unordered_map>
#include <unordered_set>

#include "db/database.hpp"
#include "db/node.hpp"

#define ALLOCATED_IDS 1024

namespace sched
{

class ids
{
   // class to hold requested node ID's and ID's that need to be deleted.
   private:

      db::node::node_id next_available_id;
      db::node::node_id end_available_id;

      db::node::node_id next_translated_id;

      db::database::map_nodes added_nodes;

      std::unordered_set<db::node::node_id, std::hash<db::node::node_id>,
         std::equal_to<db::node::node_id>, mem::allocator<db::node::node_id>>
            removed_nodes;

      size_t next_allocation;

      void allocate_more_ids(void);

   public:

      void merge(ids&);

      void commit(void);

      db::node *create_node(void);

      void delete_node(db::node *);

      explicit ids(void);
};

}

#endif
