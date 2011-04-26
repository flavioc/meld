
#include "vm/defs.hpp"
#include "db/database.hpp"
#include "process/router.hpp"

using namespace db;
using namespace std;
using namespace vm;
using namespace process;

namespace db
{
   
size_t database::nodes_total = 0;

database::database(ifstream& fp, router *rout)
{
   static const size_t node_size(sizeof(node::node_id) * 2);
   
   int_val num_nodes;
   node::node_id fake_id;
   node::node_id real_id;
   
   fp.read((char*)&num_nodes, sizeof(int_val));
   
   nodes_total = num_nodes;
   
   rout->set_nodes_total(nodes_total); // can throw database_error
   
   const size_t nodes_to_skip(remote::self->get_nodes_base());
   
   if(nodes_to_skip > 0)
      fp.seekg(node_size * nodes_to_skip, ios_base::cur);
   
   size_t nodes_to_read = remote::self->get_total_nodes();
      
   for(size_t i(0); i < nodes_to_read; ++i) {
      fp.read((char*)&fake_id, sizeof(node::node_id));
      fp.read((char*)&real_id, sizeof(node::node_id));
      
      translation[fake_id] = real_id;
      add_node(fake_id, real_id); 
   }
   
   if(!remote::i_am_last_one()) {
      const size_t nodes_left(nodes_total - (nodes_to_skip + nodes_to_read));
      
      if(nodes_left > 0)
         fp.seekg(node_size * nodes_left, ios_base::cur);
      remote::rout(cout) << "skip last " << nodes_left << " nodes" << endl;
   }
}

database::~database(void)
{
   for(map_nodes::iterator it(nodes.begin()); it != nodes.end(); ++it)
      delete it->second;
}

node*
database::find_node(const node::node_id id) const
{
   map_nodes::const_iterator it(nodes.find(id));
   
   if(it == nodes.end())
      return NULL;
   else
      return it->second;
}

node*
database::add_node(const node::node_id id, const node::node_id trans)
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
   for(map_nodes::const_iterator it(nodes.begin());
      it != nodes.end();
      ++it)
   {
      cout << *(it->second) << endl;
   }
}

void
database::dump_db(ostream& cout) const
{
   for(map_nodes::const_iterator it(nodes.begin());
      it != nodes.end();
      ++it)
   {
      it->second->dump(cout);
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
