
#include "vm/defs.hpp"
#include "db/database.hpp"

using namespace db;
using namespace std;
using namespace vm;

database::database(ifstream& fp)
{
   int_val num_nodes;
   node_id id;
   
   fp.read((char*)&num_nodes, sizeof(int_val));
   
   for(int_val i = 0; i < num_nodes; ++i) {
      fp.read((char*)&id, sizeof(node_id));
      
      add_node(id); 
   }
}

node*
database::find_node(const node_id id) const
{
   map_nodes::const_iterator it(nodes.find(id));
   
   if(it == nodes.end())
      return NULL;
   else
      return it->second;
}

bool
database::add_edge(const node_id id1, const node_id id2)
{
   node *node1(find_node(id1));
   
   if(node1 == NULL)
      return false;
      
   node *node2(find_node(id2));
   
   if(node2 == NULL)
      return false;
      
   node1->add_edge(node2);
   
   return true;
}

node*
database::add_node(const node_id id)
{
   node *ret(find_node(id));
   
   if(ret == NULL) {
      ret = new node(id);
      nodes[id] = ret;
   }
   
   return ret;
}