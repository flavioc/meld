
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <map>
#include <fstream>
#include <ostream>
#include <tr1/unordered_map>
#include <stdexcept>

#include "db/node.hpp"

namespace process {
   class router;
}

namespace db
{

class database
{
public:
   
   typedef std::tr1::unordered_map<node::node_id, node::node_id> map_translate;
   
   typedef std::map<node::node_id, node*> map_nodes;
   
private:
   
   map_nodes nodes;
   map_translate translation;
   
public:
   
   static size_t nodes_total;
   
   map_nodes::const_iterator nodes_begin(void) const { return nodes.begin(); }
   
   map_nodes::const_iterator nodes_end(void) const { return nodes.end(); }
   
   size_t num_nodes(void) const { return nodes.size(); }
   
   node* find_node(const node::node_id) const;
   
   node* add_node(const node::node_id, const node::node_id);
   
   void print_db(std::ostream&) const;
   void dump_db(std::ostream&) const;
   
   void print(std::ostream&) const;
   
   explicit database(std::ifstream&, process::router *);
   
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
