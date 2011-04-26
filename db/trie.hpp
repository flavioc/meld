
#ifndef DB_TRIE_HPP
#define DB_TRIE_HPP

#include <list>
#include <ostream>

#include "mem/base.hpp"
#include "vm/tuple.hpp"
#include "vm/defs.hpp"
#include "db/tuple.hpp"

namespace db
{
 
class trie_node: public mem::base<trie_node>
{
private:
   
   trie_node *parent;
   trie_node *sibling;
   trie_node *child;
   
   vm::tuple_field data;
   
public:
   
   inline const bool is_leaf(void) const { return (vm::ptr_val)child & 0x1; }
   
   inline void make_leaf(void) { child = (trie_node*)((vm::ptr_val)child | 0x1); }
   
   inline trie_node* get_sibling(void) { return sibling; }
   inline trie_node* get_child(void) { return (trie_node*)((vm::ptr_val)child & (~(vm::ptr_val)(0x1))); }
   inline trie_node* get_parent(void) { return parent; }
   
   explicit trie_node(const vm::tuple_field& _data):
      parent(NULL),
      sibling(NULL),
      child(NULL),
      data(_data)
   {
   }
};

typedef std::list<simple_tuple*, mem::allocator<simple_tuple*> > simple_tuple_list;
typedef std::vector<vm::tuple*, mem::allocator<vm::tuple*> > tuple_vector;

class trie
{
private:
   
   typedef trie_node node;
   
   node *child;
   
   simple_tuple_list list;
   
   simple_tuple* look_for_simple_tuple(const simple_tuple_list&, vm::tuple *);
   void commit_delete(simple_tuple_list::iterator);
   
public:
   
   class delete_info
   {
   private:

      trie *tr;
      bool to_del;
      simple_tuple_list::iterator it;

   public:

      inline const bool to_delete(void) const { return to_del; }

      void operator()(void)
      {
         tr->commit_delete(it);
      }
      
      explicit delete_info(trie *_tr,
            const bool _to_del,
            simple_tuple_list::iterator _it):
         tr(_tr), to_del(_to_del), it(_it)
      {
      }
      
      explicit delete_info(const bool):
         tr(NULL), to_del(false)
      {
      }
   };
   
   bool insert_tuple(vm::tuple *, const vm::ref_count);
   
   delete_info delete_tuple(vm::tuple *, const vm::ref_count);
   
   tuple_vector* match_predicate(void) const;
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
   
   explicit trie(void) {}
   
   ~trie(void);
};
   
}

#endif