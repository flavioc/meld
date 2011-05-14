
#ifndef SCHED_QUEUE_BOUNDED_PQUEUE_HPP
#define SCHED_QUEUE_BOUNDED_PQUEUE_HPP

#include <vector>

#include "sched/queue/safe_queue.hpp"
#include "utils/atomic.hpp"
#include "utils/utils.hpp"

namespace sched
{
   
template <class C> // parameter is a container
class queue_tree_node
{
public:
   typedef queue_tree_node<C> tree_node;
   
   utils::atomic<int> counter;
   tree_node *parent, *left, *right;
   C bin;
   
   inline const bool is_leaf(void) const { return right == NULL; }
   
   explicit queue_tree_node(void):
      counter(0),
      parent(NULL),
      left(NULL),
      right(NULL)
   {
   }
   
   ~queue_tree_node(void)
   {
      assert(counter == 0);
   }
};
   
template <class T>
class bounded_pqueue
{
private:
   
   typedef queue_tree_node< safe_queue<T> > tree_node;
   
   std::vector<tree_node*> leaves;
   tree_node *root;
   
   utils::atomic<size_t> total;
   
   tree_node* build_tree(const size_t height, const size_t prio)
   {
      tree_node *root = new tree_node();
      if(height == 0) {
         assert(prio < leaves.size());
         leaves[prio] = root;
      } else {
         root->left = build_tree(height - 1, 2 * prio);
         root->right = build_tree(height - 1, (2 * prio) + 1);
         root->left->parent = root->right->parent = root;
      }
      
      return root;
   }
   
   
public:
   
   inline const bool empty(void) const { return total == 0; }
   
   void push(const T item, const size_t prio)
   {
      assert(total >= 0);
      assert(prio >= 0);
      assert(prio < leaves.size());
      
      tree_node *node = leaves[prio];
      node->bin.push(item);
      
      while(node != root) {
         tree_node *parent = node->parent;
         if(node == parent->left) // increment if ascending from left
            parent->counter++;
         node = parent;
      }
      
      total++; // has more tasks
   }
   
   T pop(void)
   {
      assert(!empty());
      
      tree_node *node = root;
      
      while(!node->is_leaf()) {
         assert(node->counter >= 0);
         
         if(node->counter > 0) {
            node->counter--;
            node = node->left;
         } else
            node = node->right;
      }
      
      assert(!node->bin.empty());
      
      T ret(node->bin.pop());
      
      assert(total > 0);
      
      total--;
      
      assert(total >= 0);
      
      return ret;
   }
   
   explicit bounded_pqueue(const size_t range):
      total(0)
   {
      assert(range > 0);
      
      const size_t number_leaves = utils::next_power2(range);
      
      /*
      printf("Range %d number of leaves %d height %d\n",
         range, number_leaves, utils::upper_log2(number_leaves));
      */ 
      assert(number_leaves >= range);
      
      leaves.resize(number_leaves);
      
      root = build_tree(utils::upper_log2(number_leaves), 0);
   }
   
   ~bounded_pqueue(void)
   {
      assert(root != NULL);
      assert(empty());
   }
};

}

#endif

