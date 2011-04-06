
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <map>
#include <fstream>
#include <ostream>
#include <unordered_map>

#include "db/node.hpp"

namespace db
{

class database
{
public:
   
   typedef std::unordered_map<node_id, node_id> map_translate;
   
   typedef std::map<node_id, node*> map_nodes;
   
private:
   
   map_nodes nodes;
   map_translate translation;
   
public:
   
   map_nodes::const_iterator nodes_begin(void) const { return nodes.begin(); }
   
   map_nodes::const_iterator nodes_end(void) const { return nodes.end(); }
   
   size_t num_nodes(void) const { return nodes.size(); }
   
   node* find_node(const node_id) const;
   
   node* add_node(const node_id, const node_id);
   
   void print_db(std::ostream& cout) const;
   
   void print(std::ostream&) const;
   
   explicit database(std::ifstream&);
};

std::ostream& operator<<(std::ostream&, const database&);
   
}

#endif
