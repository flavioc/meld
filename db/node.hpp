
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
#include "db/tuple_aggregate.hpp"
#include "mem/allocator.hpp"
#include "db/trie.hpp"
#include "vm/defs.hpp"
#include "utils/atomic.hpp"

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
   std::vector< utils::atomic<size_t> > to_proc;
   
   trie& get_storage(const vm::predicate_id&);
	
public:
   
   inline bool no_more_to_process(const vm::predicate_id id) { return to_proc[id] == 0; }
   inline void more_to_process(const vm::predicate_id id) { to_proc[id]++; }
   inline void less_to_process(const vm::predicate_id id) { to_proc[id]--; }
   
   inline void init_to_process(const size_t num_pred) {
      for(size_t i(0); i < num_pred; ++i) {
         to_proc.push_back(utils::atomic<size_t>(0));
      }
   }
   
   inline node_id get_id(void) const { return id; }
   inline node_id get_translated_id(void) const { return translation; }
   
   bool add_tuple(vm::tuple*, vm::ref_count);
   delete_info delete_tuple(vm::tuple *, vm::ref_count);
   
   db::agg_configuration* add_agg_tuple(vm::tuple*, const vm::ref_count);
   void remove_agg_tuple(vm::tuple*, const vm::ref_count);
   simple_tuple_list generate_aggs(void);
   
   tuple_vector* match_predicate(const vm::predicate_id) const;
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
   
   explicit node(const node_id _id, const node_id _trans):
      id(_id), translation(_trans)
   {
   }
   
   virtual ~node(void);
};

std::ostream& operator<<(std::ostream&, const node&);

}

#endif

