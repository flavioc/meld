
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
   
database::database(istream& fp)
{
   int_val num_nodes;
   node::node_id fake_id;
   node::node_id user_id;
   
   fp.read((char*)&num_nodes, sizeof(int_val));
   
   nodes_total = num_nodes;
   
   max_node_id = -1;
   max_translated_id = -1;
      
   for(size_t i(0); i < nodes_total; ++i) {
      fp.read((char*)&fake_id, sizeof(node::node_id));
      fp.read((char*)&user_id, sizeof(node::node_id));
      
      // nodes themselves are created by each thread in sched/init_node.
      nodes[fake_id] = (db::node*)user_id;

      if(fake_id > max_node_id || max_node_id == (db::node::node_id)-1)
         max_node_id = fake_id;
      if(user_id > max_translated_id || max_translated_id == (db::node::node_id)-1)
         max_translated_id = user_id;
   }
   
   original_max_node_id = max_node_id;
}

void
database::wipeout(candidate_gc_nodes& gc_nodes)
{
   deleting = true;
   for(auto & elem : nodes) {
      db::node *n(elem.second);
      n->wipeout(gc_nodes);
      n->deallocate();
   }
}

node*
database::create_node_id(const db::node::node_id id)
{
   MUTEX_LOCK_GUARD(mtx, main_db_lock);

   if(max_node_id > 0) {
      assert(max_node_id < id);
      assert(max_translated_id < id);
   }

   max_node_id = id;
   max_translated_id = id;

   node *ret(node::create(max_node_id, max_translated_id));

   nodes[max_node_id] = ret;
   nodes_total++;

   return ret;
}

node*
database::create_node_iterator(database::map_nodes::iterator it)
{
   it->second = node::create(it->first, (node::node_id)it->second);
   return it->second;
}

pair<node::node_id, node::node_id>
database::allocate_ids(const size_t size)
{
   MUTEX_LOCK_GUARD(mtx, main_db_lock);

   pair<node::node_id, node::node_id> ret(
         make_pair(max_node_id + 1, max_translated_id + 1));

   max_node_id += size;
   max_translated_id += size;

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
   for(const auto & elem : nodes) {
      db::node *n(elem.second);
      total += n->count_total_all();
   }
   return total;
}

void
database::print_db(ostream& cout) const
{
   std::vector<db::node*> arr(num_nodes(), nullptr);

   size_t i(0);
   for(const auto & elem : nodes)
   {
      arr[i++] = elem.second;
   }

   sort(arr.begin(), arr.end(), node_sorter);
   for(auto & elem : arr) {
      cout << *elem << endl;
   }
}

void
database::dump_db(ostream& cout) const
{
   for(const auto & elem : nodes)
   {
      elem.second->dump(cout);
   }
}

void
database::print(ostream& cout) const
{
   cout << "{";
   for(auto it(nodes.begin());
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
