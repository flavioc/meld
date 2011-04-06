
#include "vm/defs.hpp"
#include "db/database.hpp"

using namespace db;
using namespace std;
using namespace vm;

namespace db
{

database::database(ifstream& fp)
{
   int_val num_nodes;
   node_id fake_id;
   node_id real_id;
   
   fp.read((char*)&num_nodes, sizeof(int_val));
   
   for(int_val i = 0; i < num_nodes; ++i) {
      fp.read((char*)&fake_id, sizeof(node_id));
      fp.read((char*)&real_id, sizeof(node_id));
      
      translation[fake_id] = real_id;
      add_node(fake_id, real_id); 
   }
   
   cout << *this << endl;
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

node*
database::add_node(const node_id id, const node_id trans)
{
   node *ret(find_node(id));
   
   if(ret == NULL) {
      ret = new node(id, trans);
      nodes[id] = ret;
   }
   
   return ret;
}

void
database::print_db(ostream& cout) const
{
   for(auto it(nodes.begin()); it != nodes.end(); ++it)
   {
      cout << *(it->second) << endl;
   }
}

void
database::print(ostream& cout) const
{
   cout << "{";
   for(map_nodes::const_iterator it(nodes.begin());
      it != nodes.end();
      ++it)
   {
      if(it != nodes.begin())
         cout << ", ";
      cout << it->first;
   }
   cout << "}";
}

ostream& operator<<(ostream& cout, const database& db)
{
   db.print(cout);
   return cout;
}

}