
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
#include "mem/allocator.hpp"
#include "db/trie.hpp"
#include "vm/defs.hpp"

namespace db {

class node
{
public:
   
   typedef vm::node_val node_id;
   
   typedef trie::delete_info delete_info;
   
private:
   
	node_id id;
   node_id translation;
	
   typedef std::map<vm::predicate_id, trie,
               std::less<vm::predicate_id>,
               mem::allocator<std::pair<const vm::predicate_id,
                                 simple_tuple_list> > > simple_tuple_map;
   typedef std::map<vm::predicate_id, tuple_aggregate*,
               std::less<vm::predicate_id>,
               mem::allocator<std::pair<const vm::predicate_id,
                                 tuple_aggregate*> > > aggregate_map;
	
   simple_tuple_map tuples;
   aggregate_map aggs;
   
   trie& get_storage(const vm::predicate_id&);
	
public:
   
   inline node_id get_id(void) const { return id; }
   inline node_id get_translated_id(void) const { return translation; }
   
   bool add_tuple(vm::tuple*, vm::ref_count);
   delete_info delete_tuple(vm::tuple *, vm::ref_count);
   
   void add_agg_tuple(vm::tuple*, const vm::ref_count);
   void remove_agg_tuple(vm::tuple*, const vm::ref_count);
   simple_tuple_list generate_aggs(void);
   
   tuple_vector* match_predicate(const vm::predicate_id) const;
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
   
   explicit node(const node_id _id, const node_id _trans):
      id(_id), translation(_trans)
   {
   }
   
   ~node(void);
};

std::ostream& operator<<(std::ostream&, const node&);

}

#endif

