
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

namespace db
{

typedef std::list<simple_tuple*, mem::allocator<simple_tuple*> > simple_tuple_list;
typedef std::vector<vm::tuple*, mem::allocator<vm::tuple*> > tuple_vector;

typedef std::stack<vm::tuple_field> val_stack;
typedef std::stack<vm::field_type> type_stack;

class trie_hash;
class trie_node;

class trie_leaf: public mem::base<trie_leaf>
{
private:
   
   friend class trie_node;
   friend class trie;
   friend class trie_iterator;
   
   trie_leaf *prev;
   trie_leaf *next;
   
   simple_tuple* tpl;
   
public:
   
   inline simple_tuple *get_tuple(void) const { return tpl; }
   
   explicit trie_leaf(simple_tuple *_tpl):
      tpl(_tpl)
   {
   }
};

class trie_node: public mem::base<trie_node>
{
protected:

   friend class trie;
   friend class trie_iterator;
   friend class trie_hash;

   trie_node *parent;
   trie_node *next;
   trie_node *prev;
   trie_node *child;

   vm::tuple_field data;
   
   bool hashed;
   trie_node **bucket;
   
   trie_node* get_by_first_int(const vm::int_val) const;
   void convert_hash(const vm::field_type&);

public:

   inline const bool is_hashed(void) const { return hashed; }
   inline const bool is_leaf(void) const { return (vm::ptr_val)child & 0x1; }

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
   
   trie_node *match(const vm::tuple_field&, const vm::field_type&, val_stack&, type_stack&, size_t&) const;
   
   trie_node *insert(const vm::tuple_field&, const vm::field_type&, val_stack&, type_stack&);
   
   explicit trie_node(const vm::tuple_field& _data):
      next(NULL),
      prev(NULL),
      child(NULL),
      data(_data),
      hashed(false),
      bucket(NULL)
   {
   }

   explicit trie_node(void): // no data
      next(NULL),
      prev(NULL),
      child(NULL),
      hashed(false),
      bucket(NULL)
   {
   }

   ~trie_node(void);
};

class trie_hash: public mem::base<trie_hash>
{
private:
   
   friend class trie;
   friend class trie_node;
   friend class trie_iterator;
   
   const vm::field_type type;
   
   trie_node *parent;
   trie_node **buckets;
   size_t num_buckets;
   size_t total;
   
   inline size_t hash_item(const size_t item) const { return item & (num_buckets-1); }
   
public:
   
   size_t count_refs(void) const;
   
   void insert_int(const vm::int_val&, trie_node *);
   void insert_float(const vm::float_val&, trie_node *);
   void insert_node(const vm::node_val&, trie_node *);
   
   trie_node *get_int(const vm::int_val&) const;
   trie_node *get_float(const vm::float_val&) const;
   trie_node *get_node(const vm::node_val&) const;
   
   void expand(void);
   
   explicit trie_hash(const vm::field_type&, trie_node*);
   
   ~trie_hash(void);
};

class trie_iterator: public mem::base<trie_node>
{
private:
   
   trie_leaf *current_leaf;
   
public:
   
   inline simple_tuple* operator*(void) const
   {
      assert(current_leaf != NULL);
      
      return current_leaf->get_tuple();
   }
   
   inline const bool operator==(const trie_iterator& it) const
   {
      return current_leaf == it.current_leaf;
   }
   
   inline const bool operator!=(const trie_iterator& it) const { return !operator==(it); }
   
   inline trie_iterator& operator++(void)
   {
      current_leaf = current_leaf->next;
      return *this;
   }
   
   inline trie_iterator operator++(int)
   {
      current_leaf = current_leaf->next;
      
      return *this;
   }
   
   explicit trie_iterator(trie_leaf *first_leaf):
      current_leaf(first_leaf)
   {
   }
   
   explicit trie_iterator(void): // end iterator
      current_leaf(NULL)
   {
   }
};

class trie
{
private:
   
   typedef trie_node node;
   
   const vm::predicate *pred;
   node *root;
   size_t total;
   trie_leaf *first_leaf;
   trie_leaf *last_leaf;
   
   trie_node* check_insert(vm::tuple *, const vm::ref_count, bool&);
   void commit_delete(trie_node *);
   void delete_branch(trie_node *);
   void delete_path(trie_node *);
   
public:
   
   typedef trie_iterator iterator;
   typedef trie_iterator const_iterator;
   
   class delete_info
   {
   private:

      trie *tr;
      bool to_del;
      trie_node *tr_node;

   public:

      inline const bool to_delete(void) const { return to_del; }

      void operator()(void)
      {
         tr->commit_delete(tr_node);
      }
      
      explicit delete_info(trie *_tr,
            const bool _to_del,
            trie_node *_tr_node):
         tr(_tr), to_del(_to_del), tr_node(_tr_node)
      {
      }
      
      explicit delete_info(const bool):
         tr(NULL), to_del(false)
      {
      }
   };
   
   // inserts tuple into the trie
   // returns false if tuple is repeated (+ ref count)
   // returns true if tuple is new
   bool insert_tuple(vm::tuple *, const vm::ref_count);
   
   // returns delete info object
   // call to_delete to know if the ref count reached zero
   // call the object to commit the deletion operation
   delete_info delete_tuple(vm::tuple *, const vm::ref_count);
   
   tuple_vector* match_predicate(void) const;
   
   inline const bool empty(void) const { return total == 0; }
   inline const size_t size(void) const { return total; }
   
   inline const_iterator begin(void) const { return trie_iterator(first_leaf); }
   inline const_iterator end(void) const { return trie_iterator(); }
   
   inline iterator begin(void) { return trie_iterator(first_leaf); }
   inline iterator end(void) { return trie_iterator(); }
   
   void delete_all(void);
   void delete_by_first_int_arg(const vm::int_val);
   void wipeout(void);
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
   
   explicit trie(const vm::predicate *);
   
   ~trie(void);
};
   
}

#endif