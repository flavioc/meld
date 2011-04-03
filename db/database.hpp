
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <map>
#include <fstream>

#include "db/node.hpp"

namespace db
{

class database
{
public:
   
   typedef std::map<node_id, node*> map_nodes;
   
private:
   
   map_nodes nodes;
   
public:
   
   map_nodes::const_iterator nodes_begin(void) const { return nodes.begin(); }
   
   map_nodes::const_iterator nodes_end(void) const { return nodes.end(); }
   
   size_t num_nodes(void) const { return nodes.size(); }
   
   node* find_node(const node_id) const;
   
   bool add_edge(const node_id, const node_id);
   
   node* add_node(const node_id);
   
   explicit database(std::ifstream&);
};
   
}

#endif
