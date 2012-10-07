
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
   
   void convert_hash(const vm::field_type&);

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
   
   trie_node *match(const vm::tuple_field&, const vm::field_type&, vm::val_stack&, vm::type_stack&, size_t&) const;
   
   trie_node *insert(const vm::tuple_field&, const vm::field_type&, vm::val_stack&, vm::type_stack&);
   
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
   
   const vm::field_type type;
   
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
   
   explicit trie_hash(const vm::field_type&, trie_node*);
   
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
   
   virtual size_t get_count(void) const = 0;
   
   virtual void add_count(const vm::ref_count many) = 0;
   
   virtual bool to_delete(void) const = 0;
   
   explicit trie_leaf(void)
   {
   }
   
   virtual ~trie_leaf(void)
   {
   }
};

class tuple_trie_leaf: public trie_leaf
{
private:
   
   friend class trie_node;
   friend class trie;
   friend class tuple_trie;
   friend class tuple_trie_iterator;
   
   simple_tuple* tpl;
   
public:

   MEM_METHODS(tuple_trie_leaf)
   
	inline vm::tuple *get_underlying_tuple(void) const { return tpl->get_tuple(); }
	
   inline simple_tuple *get_tuple(void) const { return tpl; }
   
   virtual inline size_t get_count(void) const { return tpl->get_count(); }
   
   virtual inline void add_count(const vm::ref_count many) { tpl->add_count(many); }
   
   virtual inline bool to_delete(void) const { return tpl->reached_zero(); }
   
   explicit tuple_trie_leaf(simple_tuple *_tpl):
      trie_leaf(),
      tpl(_tpl)
   {
   }
   
   virtual ~tuple_trie_leaf(void)
   {
      simple_tuple::wipeout(tpl);
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
   
   inline simple_tuple* operator*(void) const
   {
      assert(current_leaf != NULL);
      
      return current_leaf->get_tuple();
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
   
   void commit_delete(trie_node *, vm::ref_count);
   size_t delete_branch(trie_node *);
   void delete_path(trie_node *);
   void sanity_check(void) const;
   
   virtual trie_leaf* create_leaf(void *data, const vm::ref_count many) = 0;
   void inner_delete_by_leaf(trie_leaf *, const vm::ref_count);
   
   trie_node *check_insert(void *, const vm::ref_count, vm::val_stack&, vm::type_stack&, bool&);
   
public:
   
   class delete_info
   {
   private:

      trie *tr;
      bool to_del;
      trie_node *tr_node;
      vm::ref_count many;

   public:

      inline bool to_delete(void) const { return to_del; }

      void operator()(void)
      {
         tr->commit_delete(tr_node, many);
      }
      
      explicit delete_info(trie *_tr,
            const bool _to_del,
            trie_node *_tr_node,
            vm::ref_count _many):
         tr(_tr), to_del(_to_del), tr_node(_tr_node), many(_many)
      {
      }
      
      explicit delete_info(const bool):
         tr(NULL), to_del(false)
      {
      }
   };
   
   inline bool empty(void) const { return number_of_references == 0; }
   inline size_t size(void) const { return number_of_references; }
   
   // if second argument is 0, the leaf is ensured to be deleted
   void delete_by_leaf(trie_leaf *);
   void delete_by_index(const vm::match&);
   void wipeout(void);
   
   explicit trie(void);
   
   virtual ~trie(void);
};

class tuple_trie: public trie, public mem::base
{
private:
   
   const vm::predicate *pred;
   
   virtual trie_leaf* create_leaf(void *data, const vm::ref_count many)
   {
      return new tuple_trie_leaf(new simple_tuple((vm::tuple*)data, many));
   }
   
   trie_node* check_insert(vm::tuple *, const vm::ref_count, bool&);
   
public:

   MEM_METHODS(tuple_trie)
   
   typedef tuple_trie_iterator iterator;
   typedef tuple_trie_iterator const_iterator;
   
   // inserts tuple into the trie
   // returns false if tuple is repeated (+ ref count)
   // returns true if tuple is new
   bool insert_tuple(vm::tuple *, const vm::ref_count);
   
   // returns delete info object
   // call to_delete to know if the ref count reached zero
   // call the object to commit the deletion operation
   delete_info delete_tuple(vm::tuple *, const vm::ref_count);
   
   inline const_iterator begin(void) const { return iterator((tuple_trie_leaf*)first_leaf); }
   inline const_iterator end(void) const { return iterator(); }
   
   inline iterator begin(void) { return iterator((tuple_trie_leaf*)first_leaf); }
   inline iterator end(void) { return iterator(); }
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
   
   void match_predicate(const vm::match&, tuple_vector&) const;
   
   void match_predicate(tuple_vector&) const;
   
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
   
public:

   MEM_METHODS(agg_trie_leaf)
   
   inline void set_conf(agg_configuration* _conf) { conf = _conf; }
   
   inline agg_configuration *get_conf(void) const { return conf; }
   
   virtual inline size_t get_count(void) const { return 1; }
   
   virtual inline void add_count(const vm::ref_count) { }
   
   virtual inline bool to_delete(void) const { return false; }
   
   explicit agg_trie_leaf(agg_configuration *_conf):
      trie_leaf(),
      conf(_conf)
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
   
   virtual trie_leaf* create_leaf(void *, const vm::ref_count)
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
