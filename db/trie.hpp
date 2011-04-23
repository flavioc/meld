
#ifdef DB_TRIE_HPP
#define DB_TRIE_HPP

#include "mem/base.hpp"
#include "vm/tuple.hpp"

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
   
   inline const bool is_leaf(void) const { return child & 0x1; }
   
   inline void make_leaf(void) { child = child | 0x1; }
   
   inline trie_node* get_sibling(void) { return sibling; }
   inline trie_node* get_child(void) { return child; }
   inline trie_node* get_parent(void) { return parent; }
   
   explicit trie_node(const T& _data):
      data(_data),
      parent(NULL),
      sibling(NULL),
      child(NULL)
   {
   }
};

class trie
{
private:
   
   typedef trie_node node;
   
   node root;
   
public:
   
   explicit trie(void) {}
};
   
}