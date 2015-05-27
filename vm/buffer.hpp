
#ifndef VM_BUFFER_HPP
#define VM_BUFFER_HPP

#include <unordered_map>
#include <map>

#include "db/node.hpp"
#include "mem/allocator.hpp"
#include "vm/buffer_node.hpp"

namespace vm
{

struct buffer_pair {
   db::node *node;
   buffer_node b;
};

struct buffer
{
#if 0
   using map_sends = std::unordered_map<const db::node*, buffer_node,
         db::node_hash,
         std::equal_to<const db::node*>,
         mem::allocator<std::pair<const db::node* const, buffer_node>>>;
#endif
   using map_sends = std::map<const db::node*, buffer_node,
         std::less<const db::node*>,
         mem::allocator<std::pair<const db::node* const, buffer_node>>>;

#define MAX_VM_BUFFER 8

   buffer_pair initial[MAX_VM_BUFFER];
   size_t size_initial{0};
   map_sends facts;

   inline bool lots_of_nodes() const { return size_initial == MAX_VM_BUFFER; }

   inline void add_into_buffer_node(buffer_node &bn, vm::tuple *tpl, vm::predicate *pred)
   {
      for(buffer_item &item : bn.ls) {
         if(item.pred == pred) {
            item.ls.push_front(tpl);
            return;
         }
      }
      buffer_item ni;
      ni.pred = pred;
      ni.ls.push_back(tpl);
      bn.ls.push_back(ni);
   }

   inline void add(db::node *n, vm::tuple *tpl, vm::predicate *pred) __attribute__((always_inline))
   {
      assert(n);
      for(size_t i(0); i < size_initial; ++i) {
         assert(initial[i].node);
         if(initial[i].node == n) {
            buffer_node &bn(initial[i].b);
            add_into_buffer_node(bn, tpl, pred);
            return;
         }
      }
      if(lots_of_nodes()) {
         auto it(facts.find(n));
         if(it == facts.end()) {
            auto i(facts.insert(std::make_pair(n, buffer_node())));
            it = i.first;
         }
         buffer_node& bn(it->second);
         add_into_buffer_node(bn, tpl, pred);
      } else {
         // add new initial
         const size_t idx(size_initial);
         size_initial++;
         initial[idx].node = n;
         buffer_node &bn(initial[idx].b);
         bn.clear();
         add_into_buffer_node(bn, tpl, pred);
      }
   }

   inline void clear()
   {
      if(lots_of_nodes())
         facts.clear();
      size_initial = 0;
   }
};

}
#endif
