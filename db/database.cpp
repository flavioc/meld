
#include "vm/defs.hpp"
#include "db/database.hpp"
#include "vm/state.hpp"

using namespace db;
using namespace std;
using namespace vm;
using namespace process;
using namespace utils;

namespace db
{
   
database::database(const string& filename, create_node_fn _create_fn):
   create_fn(_create_fn), nodes_total(0)
{
   int_val num_nodes;
   node::node_id fake_id;
   node::node_id user_id;
   
   ifstream fp(filename.c_str(), ios::in | ios::binary);

   fp.seekg(vm::MAGIC_SIZE, ios_base::cur); // skip magic
   fp.seekg(2*sizeof(uint32_t), ios_base::cur); // skip version
   
   fp.seekg(sizeof(byte), ios_base::cur); // skip number of definitions
   
   fp.read((char*)&num_nodes, sizeof(int_val));
   
   nodes_total = num_nodes;
   
   if(nodes_total == 0)
      throw database_error("The program has no nodes to run.");
   
   max_node_id = 0;
   max_translated_id = 0;
      
   for(size_t i(0); i < nodes_total; ++i) {
      fp.read((char*)&fake_id, sizeof(node::node_id));
      fp.read((char*)&user_id, sizeof(node::node_id));
      
      // nodes themselves are created by each thread in sched/init_node.
      nodes[fake_id] = (db::node*)user_id;

      if(fake_id > max_node_id)
         max_node_id = fake_id;
      if(user_id > max_translated_id)
         max_translated_id = user_id;
   }
   
   original_max_node_id = max_node_id;
}

void
database::wipeout(
#ifdef GC_NODES
      candidate_gc_nodes& gc_nodes
#endif
      )
{
   for(map_nodes::iterator it(nodes.begin()); it != nodes.end(); ++it) {
#ifdef GC_NODES
      it->second->wipeout(gc_nodes);
#else
      it->second->wipeout();
#endif
      delete it->second;
   }
}

node*
database::find_node(const node::node_id id) const
{
   map_nodes::const_iterator it(nodes.find(id));

   if(it == nodes.end()) {
      cerr << "Could not find node with id " << id << endl;
      abort();
   }
   
   return it->second;
}

node*
database::create_node_id(const db::node::node_id id)
{
   LOCK_STACK(dblock);
   mtx.lock(LOCK_STACK_USE(dblock));

   if(max_node_id > 0) {
      assert(max_node_id < id);
      assert(max_translated_id < id);
   }

   max_node_id = id;
   max_translated_id = id;

   node *ret(create_fn(max_node_id, max_translated_id));

   assert(ret->is_empty());
   
   nodes[max_node_id] = ret;
   nodes_total++;
   mtx.unlock(LOCK_STACK_USE(dblock));

   return ret;
}

node*
database::create_node_iterator(database::map_nodes::iterator it)
{
   it->second = create_fn(it->first, (node::node_id)it->second);
   return it->second;
}

pair<node::node_id, node::node_id>
database::allocate_ids(const size_t size)
{
   LOCK_STACK(dblock);
   mtx.lock(LOCK_STACK_USE(dblock));

   pair<node::node_id, node::node_id> ret(
         make_pair(max_node_id + 1, max_translated_id + 1));

   max_node_id += size;
   max_translated_id += size;
   mtx.unlock(LOCK_STACK_USE(dblock));

   return ret;
}

static bool
node_sorter(db::node *a1, db::node *a2)
{
   return a1->get_translated_id() < a2->get_translated_id();
}

size_t
database::total_facts(void) const
{
   size_t total(0);
   for(map_nodes::const_iterator it(nodes.begin()); it != nodes.end(); ++it) {
      db::node *n(it->second);
      total += n->count_total_all();
   }
   return total;
}

void
database::print_db(ostream& cout) const
{
   std::vector<db::node*> arr(num_nodes(), NULL);

   size_t i(0);
   for(map_nodes::const_iterator it(nodes.begin());
      it != nodes.end();
      ++it)
   {
      arr[i++] = it->second;
   }

   sort(arr.begin(), arr.end(), node_sorter);
   for(size_t i(0); i < arr.size(); ++i) {
      cout << *arr[i] << endl;
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

#ifdef USE_UI
using namespace json_spirit;

Value
database::dump_json(void) const
{
	Object root;
	
	UI_ADD_FIELD(root, "num_nodes", (int)num_nodes());
	
	Array nodes_data;
	
	for(map_nodes::const_iterator it(nodes.begin()), end(nodes.end()); it != end; it++) {
		Object node_data;
		const node *n(it->second);
		
		UI_ADD_FIELD(node_data, "id", (int)n->get_id());
		UI_ADD_FIELD(node_data, "translated_id", (int)n->get_translated_id());

		UI_ADD_ELEM(nodes_data, node_data);
	}
	
	UI_ADD_FIELD(root, "nodes", nodes_data);
	
	return root;
}
#endif

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
