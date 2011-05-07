
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <map>
#include <fstream>
#include <ostream>
#include <tr1/unordered_map>
#include <stdexcept>
#include <boost/function.hpp>

#include "db/node.hpp"

namespace db
{

class database
{
public:
   
   typedef std::tr1::unordered_map<node::node_id, node::node_id> map_translate;
   typedef std::map<node::node_id, node*> map_nodes;
   typedef boost::function2<node*, node::node_id, node::node_id> create_node_fn;
   
private:
   
   map_nodes nodes;
   map_translate translation;
   
public:
   
   static const size_t node_size = sizeof(node::node_id) * 2;
   static size_t nodes_total;
   
   map_nodes::const_iterator nodes_begin(void) const { return nodes.begin(); }
   map_nodes::const_iterator nodes_end(void) const { return nodes.end(); }
   map_nodes::iterator get_node_iterator(const node::node_id id) { return nodes.find(id); }
   
   size_t num_nodes(void) const { return nodes.size(); }
   
   node* find_node(const node::node_id) const;
   
   void print_db(std::ostream&) const;
   void dump_db(std::ostream&) const;
   
   void print(std::ostream&) const;
   
   explicit database(const std::string&, create_node_fn);
   
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
