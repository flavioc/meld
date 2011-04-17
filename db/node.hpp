
#ifndef NODE_HPP
#define NODE_HPP

#include <list>
#include <map>
#include <ostream>
#include <vector>
#include <utility>

#include "vm/tuple.hpp"
#include "vm/predicate.hpp"
#include "db/tuple.hpp"

namespace db {

class node
{
public:
   
   typedef unsigned long int node_id;
   
   typedef std::list<stuple*> stuple_list;
   typedef std::vector<vm::tuple*> tuple_vector;
   
   typedef struct {
      bool to_delete;
      stuple_list *list;
      stuple_list::iterator it;
   } delete_info;
   
private:
   
	node_id id;
   node_id translation;
	
   typedef std::map<vm::predicate_id, stuple_list> stuple_map;
	
   stuple_map tuples;
   
   stuple_list& get_storage(const vm::predicate_id&);
	
public:
   
   inline node_id real_id(void) const { return (node_id)this; }
   inline node_id get_id(void) const { return id; }
   inline node_id get_translated_id(void) const { return translation; }
   
   bool add_tuple(vm::tuple*, vm::ref_count);
   delete_info delete_tuple(vm::tuple *, vm::ref_count);
   
   void commit_delete(const delete_info&);
   
   tuple_vector* match_predicate(const vm::predicate_id) const;
   
   void print(std::ostream&) const;
   
   explicit node(const node_id _id, const node_id _trans):
      id(_id), translation(_trans)
   {
   }
   
   ~node(void);
};

std::ostream& operator<<(std::ostream&, const node&);

}

#endif

