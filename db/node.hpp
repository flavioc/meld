
#ifndef NODE_HPP
#define NODE_HPP

#include <list>
#include <map>
#include <ostream>
#include <vector>
#include <boost/thread/mutex.hpp>
#include <utility>

#include "vm/predicate.hpp"
#include "db/tuple.hpp"

namespace db {

typedef unsigned long int node_id;

class node
{
public:
   
   typedef std::list<stuple*> stuple_list;
   typedef std::vector<vm::tuple*> tuple_vector;
   typedef std::list<node*> node_list;
   
   typedef struct {
      bool to_delete;
      stuple_list *list;
      stuple_list::iterator it;
   } delete_info;
   
private:
   
	node_id id;

	node_list edges;
	
   typedef std::list<simple_tuple*> simple_tuple_list;
   typedef std::map<vm::predicate_id, stuple_list> stuple_map;
	
   stuple_map tuples;
   simple_tuple_list queue;
   
   boost::mutex queue_mutex;
   
   stuple_list& get_storage(const vm::predicate_id&);
	
public:
   
   inline node_id real_id(void) const { return (node_id)this; }
   inline node_id get_id(void) const { return id; }
   
   void add_edge(node *node);
   
   void enqueue_tuple(simple_tuple*);
   simple_tuple* dequeue_tuple(void);
   bool queue_empty(void) const { return queue.empty(); }
   
   bool add_tuple(vm::tuple *, vm::ref_count);
   delete_info delete_tuple(vm::tuple *, vm::ref_count);
   
   void commit_delete(const delete_info&);
   
   tuple_vector* match_predicate(const vm::predicate_id) const;
   
   node_list::const_iterator edges_begin(void) const { return edges.begin(); }
   node_list::const_iterator edges_end(void) const { return edges.end(); }
   
   void print(std::ostream&) const;
   
   explicit node(const node_id _id): id(_id) {}

};

std::ostream& operator<<(std::ostream&, const node&);

}

#endif

