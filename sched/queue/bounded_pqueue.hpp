
#ifndef SCHED_QUEUE_BOUNDED_PQUEUE_HPP
#define SCHED_QUEUE_BOUNDED_PQUEUE_HPP

#include <vector>

#include "sched/queue/safe_queue.hpp"
#include "sched/queue/simple_linear_pqueue.hpp"
#include "utils/atomic.hpp"
#include "utils/utils.hpp"

namespace sched
{
   
template <class C, class A> // parameter is a container and counter
class queue_tree_node
{
public:
   typedef queue_tree_node<C, A> tree_node;
   
   A counter;
   tree_node *parent, *left, *right;
   C bin;
   
   void delete_all(void)
   {
      if(!is_leaf()) {
         left->delete_all();
         delete left;
         right->delete_all();
         delete right;
      }
   }
   
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
   
template <class T, class C, class A>
class bounded_pqueue
{
private:
   
   typedef queue_tree_node<C, A> tree_node;
   
   std::vector<tree_node*> leaves;
   tree_node *root;
   
   A total;
   size_t range;
   
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
   
   void push_queue(unsafe_queue_count<T>& q, const size_t prio)
   {
      const size_t size(q.size());
      
      assert(size > 0);
      assert(prio >= 0);
      assert(prio < leaves.size());
      
      tree_node *node(leaves[prio]);
      node->bin.snap(q);
      
      while(node != root) {
         tree_node *parent = node->parent;
         if(node == parent->left)
            parent->counter += size;
         node = parent;
      }
      
      total += size;
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
   
   void snap(simple_linear_pqueue<T>& other)
   {
      assert(!other.empty());
      assert(other.size() > 0);
      
      for(size_t i(0); i < range; ++i) {
         unsafe_queue_count<T>& q(other.get_queue(i));
         
         if(!q.empty())
            push_queue(q, i);
      }
   }
   
   explicit bounded_pqueue(const size_t _range):
      total(0),
      range(_range)
   {
      assert(range > 0);
      
      const size_t number_leaves = utils::next_power2(range);
      
      assert(number_leaves >= range);
      
      leaves.resize(number_leaves);
      
      root = build_tree(utils::upper_log2(number_leaves), 0);
   }
   
   ~bounded_pqueue(void)
   {
      assert(root != NULL);
      assert(empty());
      
      root->delete_all();
   }
};

// XXX: must use C++0X

template<typename T>
struct safe_bounded_pqueue
{
   typedef bounded_pqueue<T, safe_queue<T>, utils::atomic<size_t> > type;
};

}

#endif

