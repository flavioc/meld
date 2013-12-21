
#ifndef NODE_HPP
#define NODE_HPP

#include "conf.hpp"

#include <list>
#include <map>
#include <ostream>
#include <vector>
#include <utility>
#include <tr1/unordered_map>

#include "vm/tuple.hpp"
#include "vm/predicate.hpp"
#include "db/tuple.hpp"
#include "db/tuple_aggregate.hpp"
#include "mem/allocator.hpp"
#include "db/trie.hpp"
#include "vm/defs.hpp"
#include "vm/match.hpp"
#include "utils/atomic.hpp"
#include "vm/rule_matcher.hpp"
#include "vm/all.hpp"
#include "db/lists.hpp"
#include "vm/temporary.hpp"

#ifdef USE_UI
#include <json_spirit.h>
#endif

namespace sched { class base; class mpi_handler; }
namespace process { class process; class machine; }

namespace db {

class node: public mem::base
{
public:
   
   typedef vm::node_val node_id;
   
   typedef trie::delete_info delete_info;
   
protected:

   vm::all *all;

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
   
   tuple_trie* get_storage(const vm::predicate*);
   
   // code to handle local stratification
   friend class sched::base;
   friend class process::process;
   friend class process::machine;
   
   sched::base *owner;
   
public:

   void assert_tries(void);
   
   inline node_id get_id(void) const { return id; }
   inline node_id get_translated_id(void) const { return translation; }

   inline void set_owner(sched::base *_owner) { owner = _owner; }
   inline sched::base *get_owner(void) const { return owner; }
   
   bool add_tuple(vm::tuple*, const vm::derivation_count, const vm::depth_t);
   delete_info delete_tuple(vm::tuple *, const vm::derivation_count, const vm::depth_t);
   
   db::agg_configuration* add_agg_tuple(vm::tuple*, const vm::derivation_count, const vm::depth_t);
   db::agg_configuration* remove_agg_tuple(vm::tuple*, const vm::derivation_count, const vm::depth_t);
   simple_tuple_list end_iteration(void);
   
   void delete_by_index(const vm::predicate*, const vm::match&);
   void delete_by_leaf(const vm::predicate*, tuple_trie_leaf*, const vm::depth_t);
   void delete_all(const vm::predicate*);
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const {}
   void init(void);
   
   db::tuple_trie::tuple_search_iterator match_predicate(const vm::predicate_id) const;
  	db::tuple_trie::tuple_search_iterator match_predicate(const vm::predicate_id, const vm::match*) const;
   
   size_t count_total(const vm::predicate_id) const;
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
#ifdef USE_UI
	json_spirit::Value dump_json(void) const;
#endif

   db::lists db;
   vm::temporary_store store;
   bool unprocessed_facts;
   
   explicit node(const node_id, const node_id, vm::all *);
   
   virtual ~node(void);
};

std::ostream& operator<<(std::ostream&, const node&);

}

#endif

