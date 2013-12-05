
#ifndef DB_TRIE_HPP
#define DB_TRIE_HPP

#include <list>
#include <stack>
#include <tr1/unordered_map>
#include <ostream>

#include "mem/base.hpp"
#include "vm/tuple.hpp"
#include "vm/defs.hpp"
#include "db/tuple.hpp"
#include "vm/predicate.hpp"
#include "vm/match.hpp"
#include "vm/types.hpp"

namespace db
{
   
class trie_hash;
class trie_leaf;
class tuple_trie_leaf;

typedef std::list<simple_tuple*, mem::allocator<simple_tuple*> > simple_tuple_list;
typedef std::vector<tuple_trie_leaf*, mem::allocator<tuple_trie_leaf*> > tuple_vector;

class trie_node: public mem::base
{
public:

   MEM_METHODS(trie_node)

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
      parent(NULL),
      next(NULL),
      prev(NULL),
      child(NULL),
      data(_data),
      hashed(false),
      bucket(NULL)
   {
      assert(next == NULL && prev == NULL && parent == NULL && child == NULL && hashed == false);
   }

   explicit trie_node(void): // no data
      parent(NULL),
      next(NULL),
      prev(NULL),
      child(NULL),
      hashed(false),
      bucket(NULL)
   {
      assert(next == NULL && prev == NULL && parent == NULL && child == NULL && hashed == false);
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
   
   trie_node *parent;
   trie_node **buckets;
   size_t num_buckets;
   size_t total;
   
   inline size_t hash_item(const size_t item) const { return item & (num_buckets-1); }
   
public:

   MEM_METHODS(trie_hash)
   
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
   
   virtual void add_new(const vm::depth_t depth, const vm::derivation_count many) = 0;

   virtual void sub(const vm::depth_t depth, const vm::derivation_count many) = 0;
   
   virtual bool to_delete(void) const = 0;
   
   explicit trie_leaf(void)
   {
   }
   
   virtual ~trie_leaf(void)
   {
   }
};

class depth_counter: public mem::base
{
   private:

      typedef std::map<vm::depth_t, vm::ref_count> map_count;
      map_count counts;

   public:

      typedef map_count::const_iterator const_iterator;

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

      inline void add(const vm::depth_t depth, const vm::derivation_count count)
      {
         assert(count > 0);

         map_count::iterator it(counts.find(depth));

         if(it == counts.end()) {
            counts[depth] = (vm::ref_count)count;
            //std::cout << "New depth " << depth << " with count " << count << "\n";
         } else {
            it->second += count;
            //std::cout << "Depth " << depth << " now with count " << it->second << "\n";
         }
      }

      // decrements a count of some depth
      // returns true if the count of such depth has gone to 0
      inline bool sub(const vm::depth_t depth, const vm::derivation_count count)
      {
         map_count::iterator it(counts.find(depth));

         assert(count < 0);
         
         if(it == counts.end())
            return true;

         if((vm::ref_count)-count > it->second)
            it->second = 0;
         else
            it->second += count; // count is < 0
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
   depth_counter *depths; // depth counter -- usually NULL
   
public:

   MEM_METHODS(tuple_trie_leaf)
   
	inline vm::tuple *get_underlying_tuple(void) const { return tpl; }
	
   virtual inline vm::ref_count get_count(void) const { return count; }

   inline bool has_depth_counter(void) const { return depths != NULL; }

   inline depth_counter *get_depth_counter(void) const { return depths; }

   inline vm::depth_t get_max_depth(void) const {
      if(depths == NULL)
         return 0;
      else
         return depths->max_depth();
   }

   inline vm::depth_t get_min_depth(void) const {
      if(depths == NULL)
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
         const vm::derivation_count many)
   {
      assert(many > 0);
      count += many;
      if(depths) {
         depths->add(depth, many);
      }
   }

   virtual inline void sub(const vm::depth_t depth,
         const vm::derivation_count many)
   {
      assert(many < 0);
      if((vm::ref_count)-many > count)
         count = 0;
      else
         count += many;
      if(depths) {
         depths->sub(depth, many);
      }
   }
   
   virtual inline bool to_delete(void) const { return count == 0; }
   
   explicit tuple_trie_leaf(simple_tuple *_tpl):
      trie_leaf(),
      tpl(_tpl->get_tuple()),
      count(0)
   {
      if(tpl->is_cycle())
         depths = new depth_counter();
      else
         depths = NULL;
      add_new(_tpl->get_depth(), _tpl->get_count());
   }
   
   virtual ~tuple_trie_leaf(void)
   {
      if(depths != NULL)
         delete depths;
      delete tpl;
   }
};

class tuple_trie_iterator: public mem::base
{
private:
   
   tuple_trie_leaf *current_leaf;
   
public:

   MEM_METHODS(tuple_trie_iterator)
   
   inline tuple_trie_leaf* get_leaf(void) const
   {
      return current_leaf;
   }
   
   inline tuple_trie_leaf* operator*(void) const
   {
      assert(current_leaf != NULL);
      
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
      current_leaf(NULL)
   {
   }
};

class trie
{
protected:
   
   typedef trie_node node;
   
   node *root;
   int number_of_references;
   trie_leaf *first_leaf;
   trie_leaf *last_leaf;
   
   inline void basic_invariants(void)
   {
      assert(root != NULL);
      
      assert(root->parent == NULL);
      
      if(number_of_references == 0) {
         assert(root->child == NULL);
      } else {
         assert(root->child != NULL);
      }
      
      assert(root->prev == NULL);
      
      assert(root->next == NULL);
      
      assert((root->child == NULL && first_leaf == NULL && last_leaf == NULL)
         || (root->child != NULL && first_leaf != NULL && last_leaf != NULL));
   }
   
   void commit_delete(trie_node *, const vm::ref_count);
   size_t delete_branch(trie_node *);
   void delete_path(trie_node *);
   void sanity_check(void) const;
   
   virtual trie_leaf* create_leaf(void *data, const vm::ref_count many, const vm::depth_t depth) = 0;
   void inner_delete_by_leaf(trie_leaf *, const vm::derivation_count, const vm::depth_t);
   
   trie_node *check_insert(void *, const vm::derivation_count, const vm::depth_t, vm::match_stack&, bool&);
   
public:
   
   class delete_info
   {
   private:

      tuple_trie_leaf *leaf;
      trie *tr;
      bool to_del;
      trie_node *tr_node;
      vm::ref_count many;

   public:

      inline bool to_delete(void) const { return to_del; }
      inline bool is_valid(void) const { return tr != NULL && leaf != NULL; }

      void operator()(void)
      {
         tr->commit_delete(tr_node, many);
      }

      inline depth_counter* get_depth_counter(void) const
      {
         return leaf->get_depth_counter();
      }

      inline vm::ref_count delete_depths_above(const vm::depth_t depth)
      {
         const vm::ref_count deleted(leaf->delete_depths_above(depth));
         many = deleted;
         if(leaf->to_delete()) {
            to_del = true;
            // number of references will be updated upon calling operator()
         } else {
            tr->number_of_references -= deleted;
         }
         return deleted;
      }
      
      explicit delete_info(tuple_trie_leaf *_leaf,
            trie *_tr,
            const bool _to_del,
            trie_node *_tr_node,
            const vm::derivation_count _many):
         leaf(_leaf), tr(_tr), to_del(_to_del),
         tr_node(_tr_node), many(_many)
      {
      }
   };
   
   inline bool empty(void) const { return number_of_references == 0; }
   inline size_t size(void) const { return number_of_references; }
   
   // if second argument is 0, the leaf is ensured to be deleted
   void delete_by_leaf(trie_leaf *, const vm::depth_t);
   void delete_by_index(const vm::match&);
   void wipeout(void);
   
   explicit trie(void);
   
   virtual ~trie(void);
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

   const vm::predicate *pred;
   
   virtual trie_leaf* create_leaf(void *data, const vm::ref_count many, const vm::depth_t depth)
   {
      return new tuple_trie_leaf(new simple_tuple((vm::tuple*)data, many, depth));
   }
   
   trie_node* check_insert(vm::tuple *, const vm::derivation_count, const vm::depth_t, bool&);

   void visit(trie_node *n) const;
   void do_visit(trie_node *, const int, std::stack<vm::type*>&) const;
   
public:

   MEM_METHODS(tuple_trie)
   
   typedef tuple_trie_iterator iterator;
   typedef tuple_trie_iterator const_iterator;

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
						next = NULL;
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
					return NULL;
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
         explicit tuple_search_iterator(void): type(USE_END), next(NULL) {}

			// iterator for use with continuation stack
         explicit tuple_search_iterator(trie_continuation_frame& frm):
				type(USE_STACK), next(NULL)
         {
				find_next(frm, true);
         }
   };

   
   // inserts tuple into the trie
   // returns false if tuple is repeated (+ ref count)
   // returns true if tuple is new
   bool insert_tuple(vm::tuple *, const vm::derivation_count, const vm::depth_t);
   
   // returns delete info object
   // call to_delete to know if the ref count reached zero
   // call the object to commit the deletion operation
   delete_info delete_tuple(vm::tuple *, const vm::derivation_count, const vm::depth_t);
   
   inline const_iterator begin(void) const { return iterator((tuple_trie_leaf*)first_leaf); }
   inline const_iterator end(void) const { return iterator(); }
   
   inline iterator begin(void) { return iterator((tuple_trie_leaf*)first_leaf); }
   inline iterator end(void) { return iterator(); }
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
   
   tuple_search_iterator match_predicate(const vm::match*) const;
   
   tuple_search_iterator match_predicate(void) const;
	static inline tuple_search_iterator match_end(void) { return tuple_search_iterator(); }
   
   explicit tuple_trie(const vm::predicate *_pred): trie(), pred(_pred) { basic_invariants(); }
   
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

   MEM_METHODS(agg_trie_leaf)
   
   inline void set_conf(agg_configuration* _conf) { conf = _conf; }
   
   inline agg_configuration *get_conf(void) const { return conf; }
   
   virtual inline vm::ref_count get_count(void) const { return count; }
   
   virtual inline void add_new(const vm::depth_t, const vm::derivation_count) { }
   virtual inline void sub(const vm::depth_t, const vm::derivation_count) { }
   
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

   MEM_METHODS(agg_trie_iterator)
   
   inline agg_configuration* operator*(void) const
   {
      assert(current_leaf != NULL);
      
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
      current_leaf(NULL)
   {
   }
};

class agg_trie: public trie, public mem::base
{
private:
   
   virtual trie_leaf* create_leaf(void *, const vm::ref_count, const vm::depth_t)
   {
      return new agg_trie_leaf(NULL);
   }
   
public:

   MEM_METHODS(agg_trie)
   
   typedef agg_trie_iterator iterator;
   typedef agg_trie_iterator const_iterator;
   
   agg_trie_leaf *find_configuration(vm::tuple *);
   
   void delete_configuration(trie_node *node);
   
   inline const_iterator begin(void) const { return iterator((agg_trie_leaf*)first_leaf); }
   inline const_iterator end(void) const { return iterator(); }
   
   inline iterator begin(void) { return iterator((agg_trie_leaf*)first_leaf); }
   inline iterator end(void) { return iterator(); }
   
   iterator erase(iterator& it);
   
   explicit agg_trie(void) {}
   
   virtual ~agg_trie(void) {}
};
   
}

#endif
