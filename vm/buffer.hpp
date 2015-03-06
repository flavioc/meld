
#ifndef VM_BUFFER_HPP
#define VM_BUFFER_HPP

#include <unordered_map>
#include <map>

#include "db/node.hpp"
#include "mem/allocator.hpp"
#include "vm/buffer_node.hpp"

namespace vm
{

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

   map_sends facts;

   inline void add(db::node *n, vm::tuple *tpl, vm::predicate *pred)
   {
      auto it(facts.find(n));
      if(it == facts.end()) {
         auto i(facts.insert(std::make_pair(n, buffer_node())));
         it = i.first;
      }
      buffer_node& bn(it->second);
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

   inline void clear()
   {
      facts.clear();
   }
};

}
#endif
