
#ifndef DB_TRIE_HPP
#define DB_TRIE_HPP

#include <list>
#include <stack>
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

class trie_node: public mem::base<trie_node>
{
private:
   
   friend class trie;
   friend class trie_iterator;
   
   typedef std::stack<vm::tuple_field> val_stack;
   typedef std::stack<vm::field_type> type_stack;

   trie_node *parent;
   trie_node *next;
   trie_node *prev;
   trie_node *child;

   vm::tuple_field data;
   
   trie_node *find_next_leaf(void) const;
   trie_node *find_first_leaf(void) const;
   size_t delete_by_first_int(const vm::int_val);
   size_t count_refs(void) const;

public:

   inline const bool is_leaf(void) const { return (vm::ptr_val)child & 0x1; }

   inline trie_node* get_next(void) const { return next; }
   inline trie_node* get_child(void) const { return (trie_node*)((vm::ptr_val)child & (~(vm::ptr_val)(0x1))); }
   inline trie_node* get_parent(void) const { return parent; }
   
   inline simple_tuple* get_tuple_leaf(void) const
   {
      return (simple_tuple *)get_child();
   }
   
   inline void set_tuple_leaf(simple_tuple *tpl)
   {
      child = (trie_node *)((vm::ptr_val)tpl | 0x1);
   }
   
   trie_node *match(const vm::tuple_field&, const vm::field_type&, val_stack&, type_stack&) const;
   
   trie_node *insert(const vm::tuple_field&, const vm::field_type&, val_stack&, type_stack&);
   
   void delete_path(void);
   
   explicit trie_node(const vm::tuple_field& _data):
      parent(NULL),
      next(NULL),
      prev(NULL),
      child(NULL),
      data(_data)
   {
   }

   explicit trie_node(void): // no data
      parent(NULL),
      next(NULL),
      prev(NULL),
      child(NULL)
   {
   }

   ~trie_node(void);
};

class trie_iterator: public mem::base<trie_node>
{
private:
   
   trie_node *current_leaf;
   
public:
   
   inline simple_tuple* operator*(void) const
   {
      assert(current_leaf != NULL);
      assert(current_leaf->is_leaf());
      
      return current_leaf->get_tuple_leaf();
   }
   
   inline const bool operator==(const trie_iterator& it) const
   {
      return current_leaf == it.current_leaf;
   }
   
   inline const bool operator!=(const trie_iterator& it) const { return !operator==(it); }
   
   inline trie_iterator& operator++(void)
   {
      current_leaf = current_leaf->find_next_leaf();
      return *this;
   }
   
   inline trie_iterator operator++(int)
   {
      current_leaf = current_leaf->find_next_leaf();
      
      return *this;
   }
   
   explicit trie_iterator(trie_node *first_leaf):
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
   
   trie_node* check_insert(vm::tuple *, const vm::ref_count, bool&);
   void commit_delete(trie_node *);
   
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
   
   inline const_iterator begin(void) const { return trie_iterator(root->find_first_leaf()); }
   inline const_iterator end(void) const { return trie_iterator(); }
   
   inline iterator begin(void) { return trie_iterator(root->find_first_leaf()); }
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