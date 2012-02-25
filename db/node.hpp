
#ifndef NODE_HPP
#define NODE_HPP

#include <list>
#include <map>
#include <ostream>
#include <vector>
#include <utility>
#include <tr1/unordered_map>

#include "vm/tuple.hpp"
#include "vm/predicate.hpp"
#include "vm/strata.hpp"
#include "db/tuple.hpp"
#include "db/tuple_aggregate.hpp"
#include "mem/allocator.hpp"
#include "db/trie.hpp"
#include "vm/defs.hpp"
#include "vm/match.hpp"
#include "utils/atomic.hpp"
#include "db/edge_set.hpp"

namespace sched { class base; class mpi_handler; }
namespace process { class process; class machine; }

namespace db {

class node
{
public:
   
   typedef vm::node_val node_id;
   
   typedef trie::delete_info delete_info;
   
private:
   
	node_id id;
   node_id translation;
	
   typedef std::map<vm::predicate_id, tuple_trie*,
               std::less<vm::predicate_id>,
               mem::allocator<std::pair<const vm::predicate_id,
                                 tuple_trie*> > > simple_tuple_map;
                                 
   typedef std::map<vm::predicate_id, tuple_aggregate*,
               std::less<vm::predicate_id>,
               mem::allocator<std::pair<const vm::predicate_id,
                                 tuple_aggregate*> > > aggregate_map;
	
	// tuple database
   simple_tuple_map tuples;
   
   // sets of tuple aggregates
   aggregate_map aggs;
   
   typedef std::tr1::unordered_map<vm::predicate_id,
                           edge_set,
                           std::tr1::hash<vm::predicate_id>,
                           std::equal_to<vm::predicate_id>,
                           mem::allocator<std::pair<const vm::predicate_id, edge_set> > >
                  edge_map; 
   
   edge_map edge_info;
   
   tuple_trie* get_storage(const vm::predicate*);
   
   // code to handle local stratification
   friend class sched::base;
   friend class process::process;
   friend class process::machine;
   
   vm::strata strats;
   
   inline void push_auto(const simple_tuple *stpl)
   {
      strats.push_queue(stpl->get_strat_level());
   }
   
   inline void pop_auto(const simple_tuple *stpl)
   {
      strats.pop_queue(stpl->get_strat_level());
   }
   
   inline vm::strat_level get_local_strat_level(void)
   {
      return strats.get_current_level();
   }
	
public:
   
   inline node_id get_id(void) const { return id; }
   inline node_id get_translated_id(void) const { return translation; }
   
   bool add_tuple(vm::tuple*, vm::ref_count);
   delete_info delete_tuple(vm::tuple *, vm::ref_count);
   
   db::agg_configuration* add_agg_tuple(vm::tuple*, const vm::ref_count);
   void remove_agg_tuple(vm::tuple*, const vm::ref_count);
   simple_tuple_list end_iteration(void);
   
   const edge_set& get_edge_set(const vm::predicate_id id) const {
      assert(edge_info.find(id) != edge_info.end());
      return edge_info.find(id)->second;
   }
   
   void delete_by_index(const vm::predicate*, const vm::match&);
   void delete_by_leaf(const vm::predicate*, tuple_trie_leaf*);
   void delete_all(const vm::predicate*);
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const {}
   void init(void);
   
   void match_predicate(const vm::predicate_id, tuple_vector&) const;
   void match_predicate(const vm::predicate_id, const vm::match&, tuple_vector&) const;
   
   size_t count_total(const vm::predicate_id) const;
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
   
   explicit node(const node_id, const node_id);
   
   virtual ~node(void);
};

std::ostream& operator<<(std::ostream&, const node&);

}

#endif

