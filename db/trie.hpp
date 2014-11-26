
#ifndef DB_TRIE_HPP
#define DB_TRIE_HPP

#include <list>
#include <stack>
#include <map>
#include <unordered_map>
#include <ostream>

#include "mem/base.hpp"
#include "vm/tuple.hpp"
#include "vm/defs.hpp"
#include "vm/full_tuple.hpp"
#include "vm/predicate.hpp"
#include "vm/match.hpp"
#include "vm/types.hpp"

namespace db
{
   
class trie_hash;
class trie_leaf;
class tuple_trie_leaf;

class trie_node: public mem::base
{
public:

   trie_node *parent;
   trie_node *next;
   trie_node *prev;
   trie_node *child;

   vm::tuple_field data;
   
   bool hashed;
   trie_node **bucket;
   
   trie_node* get_by_int(const vm::int_val) const;
   trie_node* get_by_float(const vm::float_val) const;
   trie_node* get_by_node(const vm::node_val) const;
   
   void convert_hash(vm::type*);

   inline bool is_hashed(void) const { return hashed; }
   inline trie_hash* get_hash(void) const { return (trie_hash*)child; }
   inline bool is_leaf(void) const { return (vm::ptr_val)child & 0x1; }

   inline trie_node* get_next(void) const { return next; }
   inline trie_node* get_child(void) const { return (trie_node*)((vm::ptr_val)child & (~(vm::ptr_val)(0x1))); }
   inline trie_node* get_parent(void) const { return parent; }

   inline trie_leaf* get_leaf(void) const
   {
      return (trie_leaf *)get_child();
   }
   
   inline void set_leaf(trie_leaf *leaf)
   {
      child = (trie_node *)((vm::ptr_val)leaf | 0x1);
   }
   
   size_t count_refs(void) const;
   
   trie_node *match(const vm::tuple_field&, vm::type*, vm::match_stack&, size_t&) const;
   
   trie_node *insert(const vm::tuple_field&, vm::type*, vm::match_stack&);
   
   explicit trie_node(const vm::tuple_field& _data):
      parent(nullptr),
      next(nullptr),
      prev(nullptr),
      child(nullptr),
      data(_data),
      hashed(false),
      bucket(nullptr)
   {
      assert(next == nullptr && prev == nullptr && parent == nullptr && child == nullptr && hashed == false);
   }

   explicit trie_node(void): // no data
      parent(nullptr),
      next(nullptr),
      prev(nullptr),
      child(nullptr),
      hashed(false),
      bucket(nullptr)
   {
      assert(next == nullptr && prev == nullptr && parent == nullptr && child == nullptr && hashed == false);
   }

   ~trie_node(void);
};

class trie_hash: public mem::base
{
private:
   
   friend class trie;
   friend class tuple_trie;
   friend class trie_node;
   friend class tuple_trie_iterator;
   
   vm::type *type;
   
   trie_node **buckets;
   size_t num_buckets;
   size_t total;
   
   inline size_t hash_item(const size_t item) const { return item % num_buckets; }
   
public:

   size_t count_refs(void) const;
   
   void insert_int(const vm::int_val&, trie_node *);
	void insert_uint(const vm::uint_val&, trie_node *);
   void insert_float(const vm::float_val&, trie_node *);
   void insert_node(const vm::node_val&, trie_node *);
   
   trie_node *get_int(const vm::int_val&) const;
   trie_node *get_float(const vm::float_val&) const;
   trie_node *get_node(const vm::node_val&) const;
	trie_node *get_uint(const vm::uint_val&) const;   

   void expand(void);
   
   explicit trie_hash(vm::type *, trie_node*);
   
   ~trie_hash(void);
};

class trie_leaf: public mem::base
{
private:
   
   friend class trie_node;
   friend class trie;
   friend class tuple_trie;
   friend class tuple_trie_iterator;
   friend class agg_trie_iterator;
   friend class agg_trie;
   
   trie_leaf *prev;
   trie_leaf *next;
   trie_node *node;
   
public:
   
   virtual vm::ref_count get_count(void) const = 0;
   
   virtual void add_new(const vm::depth_t depth, const vm::ref_count many) = 0;

   virtual void sub(const vm::depth_t depth, const vm::ref_count many) = 0;
   
   virtual bool to_delete(void) const = 0;
   
   explicit trie_leaf(void)
   {
   }

   virtual void destroy(vm::predicate *, vm::candidate_gc_nodes&) {}
};

class depth_counter: public mem::base
{
   private:

      using map_count = std::map<vm::depth_t, vm::ref_count>;
      map_count counts;

   public:

      using const_iterator = map_count::const_iterator;

      inline bool empty(void) const
      {
         return counts.empty();
      }

      inline vm::ref_count get_count(const vm::depth_t depth) const
      {
         map_count::const_iterator it(counts.find(depth));
         if(it == counts.end())
            return 0;
         else
            return it->second;
      }

      inline const_iterator begin(void) const { return counts.begin(); }
      inline const_iterator end(void) const { return counts.end(); }

      inline vm::depth_t max_depth(void) const
      {
         map_count::const_reverse_iterator it(counts.rbegin());
         if(it == counts.rend())
            return 0;
         return it->first;
      }

      inline vm::depth_t min_depth(void) const
      {
         map_count::const_iterator it(counts.begin());
         if(it == counts.end())
            return 0;
         return it->first;
      }

      inline void add(const vm::depth_t depth, const vm::ref_count count)
      {
         assert(count > 0);

         map_count::iterator it(counts.find(depth));

         if(it == counts.end()) {
            counts[depth] = count;
            //std::cout << "New depth " << depth << " with count " << count << "\n";
         } else {
            it->second += count;
            //std::cout << "Depth " << depth << " now with count " << it->second << "\n";
         }
      }

      // decrements a count of some depth
      // returns true if the count of such depth has gone to 0
      inline bool sub(const vm::depth_t depth, const vm::ref_count count)
      {
         map_count::iterator it(counts.find(depth));

         assert(count > 0);
         
         if(it == counts.end())
            return true;

         if(count > it->second)
            it->second = 0;
         else
            it->second -= count; // count is < 0
         //std::cout << "Depth " << depth << " dropped to count " << it->second << "\n";

         if(it->second == 0) {
            //std::cout << "Erasing depth " << depth << "\n";
            counts.erase(it);
            return true;
         }
         return false;
      }

      // deletes all references above a certain depth
      // and returns the number of references deleted
      inline vm::ref_count delete_depths_above(const vm::depth_t depth)
      {
         vm::ref_count ret(0);
         while(true) {
            map_count::reverse_iterator it(counts.rbegin());

            if(it == counts.rend())
               return ret;
            if(it->first <= depth)
               return ret;
            ret += it->second;
            counts.erase(it->first);
         }
         return ret;
      }

      explicit depth_counter(void) {}
};

class tuple_trie_leaf: public trie_leaf
{
private:
   
   friend class trie_node;
   friend class trie;
   friend class tuple_trie;
   friend class tuple_trie_iterator;
   
   vm::tuple *tpl;
   vm::ref_count count;
   /// XXX this may be deleted...
   vm::ref_count used; // this is utilized by the VM core to manage leaves
   depth_counter *depths; // depth counter -- usually nullptr
   
public:

	inline vm::tuple *get_underlying_tuple(void) const { return tpl; }
	
   virtual inline vm::ref_count get_count(void) const { return count; }

   inline bool has_depth_counter(void) const { return depths != nullptr; }

   inline depth_counter *get_depth_counter(void) const { return depths; }

   inline bool new_ref_use(void) {
      if(used == count)
         return false;
      used++;
      return true;
   }

   inline void delete_ref_use(void) {
      assert(used > 0);
      used--;
   }

   inline void reset_ref_use(void) {
      used = 0;
   }

   inline vm::depth_t get_max_depth(void) const {
      if(depths == nullptr)
         return 0;
      else
         return depths->max_depth();
   }

   inline vm::depth_t get_min_depth(void) const {
      if(depths == nullptr)
         return 0;
      else
         return depths->min_depth();
   }

   inline depth_counter::const_iterator get_depth_begin(void) const { return depths->begin(); }
   inline depth_counter::const_iterator get_depth_end(void) const { return depths->end(); }

   inline vm::ref_count delete_depths_above(const vm::depth_t depth)
   {
      assert(depths);
      vm::ref_count ret(depths->delete_depths_above(depth));

      count -= ret;
      return ret;
   }
   
   virtual inline void add_new(const vm::depth_t depth,
         const vm::ref_count many)
   {
      assert(many > 0);
      count += many;
      if(depths) {
         depths->add(depth, many);
      }
   }

   inline void checked_sub(const vm::ref_count many)
   {
      assert(many > 0);
      assert(count >= many);
      assert(depths == nullptr);
      count -= many;
   }

   virtual inline void sub(const vm::depth_t depth,
         const vm::ref_count many)
   {
      assert(many > 0);
      if(many > count)
         count = 0;
      else
         count -= many;
      if(depths)
         depths->sub(depth, many);
   }
   
   virtual inline bool to_delete(void) const { return count == 0; }
   
   explicit tuple_trie_leaf(vm::tuple *_tpl, vm::predicate *pred, const vm::ref_count count, const vm::depth_t depth):
      trie_leaf(),
      tpl(_tpl),
      count(0),
      used(0)
   {
      if(pred->is_cycle_pred())
         depths = new depth_counter();
      else
         depths = nullptr;
      add_new(depth, count);
   }

   virtual void destroy(vm::predicate *pred, vm::candidate_gc_nodes& gc_nodes)
   {
      if(depths)
         delete depths;
      vm::tuple::destroy(tpl, pred, gc_nodes);
   }
};

class tuple_trie_iterator: public mem::base
{
private:
   
   tuple_trie_leaf *current_leaf;
   
public:

   inline tuple_trie_leaf* get_leaf(void) const
   {
      return current_leaf;
   }
   
   inline tuple_trie_leaf* operator*(void) const
   {
      assert(current_leaf != nullptr);
      
      return current_leaf;
   }
   
   inline bool operator==(const tuple_trie_iterator& it) const
   {
      return current_leaf == it.current_leaf;
   }
   
   inline bool operator!=(const tuple_trie_iterator& it) const { return !operator==(it); }
   
   inline tuple_trie_iterator& operator++(void)
   {
      current_leaf = (tuple_trie_leaf*)current_leaf->next;
      return *this;
   }
   
   inline tuple_trie_iterator operator++(int)
   {
      current_leaf = (tuple_trie_leaf*)current_leaf->next;
      
      return *this;
   }
   
   explicit tuple_trie_iterator(tuple_trie_leaf *first_leaf):
      current_leaf(first_leaf)
   {
   }
   
   explicit tuple_trie_iterator(void): // end iterator
      current_leaf(nullptr)
   {
   }
};

class trie
{
protected:
   
   using node = trie_node;
   
   node *root;
   int number_of_references;
   trie_leaf *first_leaf;
   trie_leaf *last_leaf;
   
   inline void basic_invariants(void)
   {
      assert(root != nullptr);
      
      assert(root->parent == nullptr);
      
      if(number_of_references == 0) {
         assert(root->child == nullptr);
      } else {
         assert(root->child != nullptr);
      }
      
      assert(root->prev == nullptr);
      
      assert(root->next == nullptr);
      
      assert((root->child == nullptr && first_leaf == nullptr && last_leaf == nullptr)
         || (root->child != nullptr && first_leaf != nullptr && last_leaf != nullptr));
   }
   
   void commit_delete(trie_node *, vm::predicate *, const vm::ref_count, vm::candidate_gc_nodes&);
   size_t delete_branch(trie_node *, vm::predicate *, vm::candidate_gc_nodes&);
   void delete_path(trie_node *);
   void sanity_check(void) const;
   
   virtual trie_leaf* create_leaf(void *data, vm::predicate*, const vm::ref_count many,
         const vm::depth_t depth) = 0;
   void inner_delete_by_leaf(trie_leaf *, vm::predicate *, const vm::ref_count,
         const vm::depth_t, vm::candidate_gc_nodes&);
   
   trie_node *check_insert(void *, vm::predicate *, const vm::derivation_direction,
         const vm::depth_t, vm::match_stack&, bool&);
   
public:
   
   class delete_info
   {
   private:

      tuple_trie_leaf *leaf;
      trie *tr;
      bool to_del;
      trie_node *tr_node;
      vm::ref_count decrease;

   public:

      inline bool to_delete(void) const { return to_del; }
      inline bool is_valid(void) const { return tr != nullptr && leaf != nullptr; }

      void perform_delete(vm::predicate *pred, vm::candidate_gc_nodes& gc_nodes)
      {
         tr->commit_delete(tr_node, pred, decrease, gc_nodes);
      }

      inline depth_counter* get_depth_counter(void) const
      {
         return leaf->get_depth_counter();
      }

      inline size_t trie_size(void) const
      {
         return tr->size();
      }

      inline vm::ref_count delete_depths_above(const vm::depth_t depth)
      {
         const vm::ref_count deleted(leaf->delete_depths_above(depth));

         decrease += deleted;
         if(leaf->to_delete())
            to_del = true;
         return deleted;
      }
      
      explicit delete_info(tuple_trie_leaf *_leaf = nullptr,
            trie *_tr = nullptr,
            const bool _to_del = false,
            trie_node *_tr_node = nullptr,
            const vm::ref_count _dec = 0):
         leaf(_leaf), tr(_tr), to_del(_to_del),
         tr_node(_tr_node), decrease(_dec)
      {
      }
   };
   
   inline bool empty(void) const { return number_of_references == 0; }
   inline size_t size(void) const { return number_of_references; }
   
   // if second argument is 0, the leaf is ensured to be deleted
   void delete_by_leaf(trie_leaf *, vm::predicate *, const vm::depth_t,
         vm::candidate_gc_nodes&);
   void delete_by_index(vm::predicate *, const vm::match&, vm::candidate_gc_nodes&);
   virtual void wipeout(vm::predicate *, vm::candidate_gc_nodes&);
   
   explicit trie(void);
};

struct trie_continuation_frame {
   vm::match_stack mstack;
	trie_node *parent;
   trie_node *node;
};

typedef utils::stack<trie_continuation_frame> trie_continuation_stack;

class tuple_trie: public trie, public mem::base
{
private:

   virtual trie_leaf* create_leaf(void *data, vm::predicate *pred, const vm::ref_count many, const vm::depth_t depth)
   {
      return new tuple_trie_leaf((vm::tuple*)data, pred, many, depth);
   }
   
   trie_node* check_insert(vm::tuple *, vm::predicate *, const vm::derivation_direction, const vm::depth_t, bool&);

   void visit(trie_node *n, vm::predicate *pred) const;
   void do_visit(trie_node *, const int, std::stack<vm::type*>&) const;
   
public:

   void assert_used(void) {
      tuple_trie_leaf *leaf((tuple_trie_leaf*)first_leaf);

      while(leaf) {
         assert(leaf->used == 0);
         leaf = (tuple_trie_leaf*)leaf->next;
      }
   }

   using iterator = tuple_trie_iterator;
   using const_iterator = tuple_trie_iterator;

	// this search iterator uses either the leaf list
	// or a continuation stack to retrieve the next valid match
	// the leaf list is used because we want all the tuples from the trie
   class tuple_search_iterator: public mem::base
   {
      public:
         
         typedef enum {
            USE_LIST,
            USE_STACK,
            USE_END
         } iterator_type;

      private:

         iterator_type type;
         trie_continuation_stack cont_stack;
			tuple_trie_leaf *next;
			
			inline void increment(void) {
            if(type == USE_LIST)
					next = (tuple_trie_leaf*)next->next;
				else if(type == USE_STACK) {
					if(cont_stack.empty())
						next = nullptr;
					else {
						trie_continuation_frame frm;
						frm = cont_stack.top();
						cont_stack.pop();
						find_next(frm);
					}
				} else
					assert(false);
			}
	
      public:
	
			void find_next(trie_continuation_frame&, const bool force_down = false);

         inline tuple_search_iterator& operator++(void)
         {
				increment();
				return *this;
         }

		   inline tuple_trie_leaf* operator*(void) const
		   {
				if(type == USE_LIST || type == USE_STACK)
					return next;
				else
					return nullptr;
		   }

         inline bool operator==(const tuple_search_iterator& it) const
         {
				return next == it.next;
         }
         
         inline bool operator!=(const tuple_search_iterator& it) const { return !operator==(it); }

         explicit tuple_search_iterator(tuple_trie_leaf *leaf):
            type(USE_LIST), next(leaf)
         {
         }

         // end iterator
         explicit tuple_search_iterator(void): type(USE_END), next(nullptr) {}

			// iterator for use with continuation stack
         explicit tuple_search_iterator(trie_continuation_frame& frm):
				type(USE_STACK), next(nullptr)
         {
				find_next(frm, true);
         }
   };

   
   // inserts tuple into the trie
   // returns false if tuple is repeated (+ ref count)
   // returns true if tuple is new
   bool insert_tuple(vm::tuple *, vm::predicate *, const vm::depth_t depth = 0);
   
   // returns delete info object
   // call to_delete to know if the ref count reached zero
   // call the object to commit the deletion operation
   delete_info delete_tuple(vm::tuple *, vm::predicate *, const vm::depth_t depth = 0);
   
   inline const_iterator begin(void) const { return iterator((tuple_trie_leaf*)first_leaf); }
   inline const_iterator end(void) const { return iterator(); }
   
   inline iterator begin(void) { return iterator((tuple_trie_leaf*)first_leaf); }
   inline iterator end(void) { return iterator(); }
   
   std::vector<std::string> get_print_strings(const vm::predicate *) const;
   void print(std::ostream&, const vm::predicate *) const;
   
   tuple_search_iterator match_predicate(const vm::match*) const;

   tuple_search_iterator match_predicate(void) const;
	static inline tuple_search_iterator match_end(void) { return tuple_search_iterator(); }
   
   explicit tuple_trie(void): trie() { basic_invariants(); }
   
   virtual ~tuple_trie(void) {}
};

class agg_configuration;

class agg_trie_leaf: public trie_leaf
{
private:
   
   friend class trie_node;
   friend class trie;
   friend class tuple_trie;
   friend class tuple_trie_iterator;
   
   agg_configuration *conf;
   vm::ref_count count;
   
public:

   inline void set_conf(agg_configuration* _conf) { conf = _conf; }
   
   inline agg_configuration *get_conf(void) const { return conf; }
   
   virtual inline vm::ref_count get_count(void) const { return count; }
   
   virtual inline void add_new(const vm::depth_t, const vm::ref_count) { }
   virtual inline void sub(const vm::depth_t, const vm::ref_count) { }
   
   inline void set_zero_refs(void) { count = 0; }

   virtual inline bool to_delete(void) const { return count == 0; }
   
   explicit agg_trie_leaf(agg_configuration *_conf):
      trie_leaf(),
      conf(_conf),
      count(1)
   {
   }
   
   virtual ~agg_trie_leaf(void);
};

class agg_trie_iterator: public mem::base
{
private:
   
   friend class agg_trie;
   
   agg_trie_leaf *current_leaf;
   
public:

   inline agg_configuration* operator*(void) const
   {
      assert(current_leaf != nullptr);
      
      return current_leaf->get_conf();
   }
   
   inline bool operator==(const agg_trie_iterator& it) const
   {
      return current_leaf == it.current_leaf;
   }
   
   inline bool operator!=(const agg_trie_iterator& it) const { return !operator==(it); }
   
   inline agg_trie_iterator& operator++(void)
   {
      current_leaf = (agg_trie_leaf*)current_leaf->next;
      return *this;
   }
   
   inline agg_trie_iterator operator++(int)
   {
      current_leaf = (agg_trie_leaf*)current_leaf->next;
      
      return *this;
   }
   
   explicit agg_trie_iterator(agg_trie_leaf *first_leaf):
      current_leaf(first_leaf)
   {
   }
   
   explicit agg_trie_iterator(void): // end iterator
      current_leaf(nullptr)
   {
   }
};

class agg_trie: public trie, public mem::base
{
private:
   
   virtual trie_leaf* create_leaf(void *, vm::predicate *, const vm::ref_count, const vm::depth_t)
   {
      return new agg_trie_leaf(nullptr);
   }
   
public:

   using iterator = agg_trie_iterator;
   using const_iterator = agg_trie_iterator;
   
   agg_trie_leaf *find_configuration(vm::tuple *, vm::predicate *);
   
   void delete_configuration(trie_node *node);
   
   inline const_iterator begin(void) const { return iterator((agg_trie_leaf*)first_leaf); }
   inline const_iterator end(void) const { return iterator(); }
   
   inline iterator begin(void) { return iterator((agg_trie_leaf*)first_leaf); }
   inline iterator end(void) { return iterator(); }
   
   iterator erase(iterator& it, vm::predicate *, vm::candidate_gc_nodes&);
   
   explicit agg_trie(void) {}
   
   virtual ~agg_trie(void) {}
};
   
}

#endif
