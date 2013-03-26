
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <map>
#include <fstream>
#include <ostream>
#include <tr1/unordered_map>
#include <stdexcept>
#include <boost/function.hpp>
#include <boost/static_assert.hpp>

#include "conf.hpp"
#include "db/node.hpp"
#include "utils/spinlock.hpp"
#include "vm/program.hpp"

#ifdef USE_UI
#include <json_spirit.h>
#include "ui/macros.hpp"
#endif

namespace db
{

class database
{
public:
   
   typedef std::tr1::unordered_map<node::node_id, node::node_id,
           std::tr1::hash<node::node_id>,
           std::equal_to<node::node_id>,
           mem::allocator< std::pair<const node::node_id, node::node_id> > > map_translate;
   typedef std::map<node::node_id, node*,
           std::less<node::node_id>,
           mem::allocator< std::pair<const node::node_id, node*> > > map_nodes;
   typedef boost::function3<node*, node::node_id, node::node_id, vm::all *> create_node_fn;

private:

   vm::all *all;
   create_node_fn create_fn;
   
   map_nodes nodes;
   map_translate translation;
   node::node_id original_max_node_id;
   node::node_id max_node_id;
   node::node_id max_translated_id;

	utils::spinlock mtx;
   
public:

   BOOST_STATIC_ASSERT(sizeof(node::node_id) == 4);

   static const size_t node_size = sizeof(node::node_id) * 2;
   size_t nodes_total;
   
   map_nodes::const_iterator nodes_begin(void) const { return nodes.begin(); }
   map_nodes::const_iterator nodes_end(void) const { return nodes.end(); }
   map_nodes::iterator get_node_iterator(const node::node_id id) { return nodes.find(id); }
   
   size_t num_nodes(void) const { return nodes.size(); }
   node::node_id max_id(void) const { return max_node_id; }
   node::node_id static_max_id(void) const { return original_max_node_id; }
   
   node* find_node(const node::node_id) const;
   node* create_node(void);
   node* create_node_id(const node::node_id);
   
   void print_db(std::ostream&) const;
   void dump_db(std::ostream&) const;
#ifdef USE_UI
	json_spirit::Value dump_json(void) const;
#endif
   
   void print(std::ostream&) const;
   
   explicit database(const std::string&, create_node_fn, vm::all *);
   
   ~database(void);
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
