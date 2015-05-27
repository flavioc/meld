
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <map>
#include <functional>
#include <fstream>
#include <ostream>
#include <unordered_map>
#include <stdexcept>

#include "db/node.hpp"
#include "vm/program.hpp"
#include "utils/mutex.hpp"

namespace db
{

class database
{
public:
   
   using map_nodes = 
      std::map<node::node_id, node*,
         std::less<node::node_id>,
         mem::allocator< std::pair<const node::node_id, node*>>>;

private:

   map_nodes nodes;
   node::node_id original_max_node_id;
   node::node_id max_node_id;
   node::node_id max_translated_id;

   utils::mutex mtx;
   // marks the database object as being deleted
   // needed because tuples may need to reference other nodes that were already deleted.
   bool deleting = false;
   
public:

   static_assert(sizeof(node::node_id) == sizeof(vm::node_val),
         "node_id must have the same size as node_val.");

   static const size_t node_size = sizeof(node::node_id) * 2;
   size_t nodes_total{0};
   
   map_nodes::const_iterator nodes_begin(void) const { return nodes.begin(); }
   map_nodes::const_iterator nodes_end(void) const { return nodes.end(); }
   map_nodes::iterator get_node_iterator(const node::node_id id) { return nodes.find(id); }
   
   size_t num_nodes(void) const { return nodes.size(); }
   node::node_id max_id(void) const { return max_node_id; }
   node::node_id static_max_id(void) const { return original_max_node_id; }
   inline bool is_deleting(void) const { return deleting; }
   inline bool is_initial_node(const db::node *n) const
   {
      if(deleting)
         return true;
      return n->get_id() <= original_max_node_id;
   }
   
   void add_node_set(map_nodes& m)
   {
      MUTEX_LOCK_GUARD(mtx, main_db_lock);
      for(auto& p : m)
         nodes[p.first] = p.second;
   }

   node* find_node(const node::node_id id) const
   {
      auto it(nodes.find(id));

      if(it == nodes.end())
         abort();
      
      return it->second;
   }

   std::pair<node::node_id, node::node_id> allocate_ids(const size_t);
   node* create_node_id(const node::node_id);
   node* create_node_iterator(map_nodes::iterator);
   
   size_t total_facts(void) const;
   void print_db(std::ostream&) const;
   void dump_db(std::ostream&) const;
   
   void print(std::ostream&) const;
   
   explicit database(std::istream&);
   
   void wipeout(vm::candidate_gc_nodes&);

   ~database(void) {
#ifdef FREE_OBJS
      vm::candidate_gc_nodes nodes;
      wipeout(nodes);
#endif
   }
};

std::ostream& operator<<(std::ostream&, const database&);

class database_error : public std::runtime_error {
 public:
    explicit database_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};
   
}

#endif
