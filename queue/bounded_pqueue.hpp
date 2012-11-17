
#ifndef QUEUE_BOUNDED_PQUEUE_HPP
#define QUEUE_BOUNDED_PQUEUE_HPP

#include <vector>

#include "queue/safe_linear_queue.hpp"
#include "queue/unsafe_linear_queue.hpp"
#include "queue/unsafe_double_queue.hpp"
#include "queue/simple_linear_pqueue.hpp"
#include "utils/atomic.hpp"
#include "utils/utils.hpp"
#include "mem/allocator.hpp"

namespace queue
{
   
template <class C, class A> // parameter is a container and a counter
class queue_tree_node: public mem::base
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
   
   inline bool is_leaf(void) const { return right == NULL; }
   
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
	typedef std::vector<tree_node*> leaves_vector;
   leaves_vector leaves;
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
   
   void push_queue(unsafe_linear_queue_count<T>& q, const size_t prio)
   {
      const size_t size(q.size());
      
      assert(size > 0);
      assert(prio < leaves.size());
      
      tree_node *node(leaves[prio]);
      node->bin.splice(q);
      
      while(node != root) {
         tree_node *parent = node->parent;
         if(node == parent->left)
            parent->counter += size;
         node = parent;
      }
      
      total += size;
   }
   
public:
   
   inline size_t size(void) const { return total; }
   
   inline bool empty(void) const { return total == 0; }
   
   void push(const T item, const size_t prio)
   {
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

	template <class Alloc>
	void top_list(std::list<T, Alloc>& vec)
	{
		if(empty())
			return;
	
		tree_node *node = root;
		
		while(!node->is_leaf()) {
			if(node->counter > 0)
				node = node->left;
			else
				node = node->right;
		}
		
		assert(!node->bin.empty());
		
		node->bin.pop_list(vec);
		
		assert(!vec.empty());
		
		const size_t size(vec.size());
		
		while(node != root) {
         tree_node *parent = node->parent;
         if(node == parent->left)
            parent->counter -= size;
         node = parent;
      }

		total -= size;
	}

	T top(void)
	{
		tree_node *node = root;
		
		while(!node->is_leaf()) {
			if(node->counter > 0)
				node = node->left;
			else
				node = node->right;
		}
		
		assert(!node->bin.empty());
		
		return node->bin.top();
	}
   
   T pop(void)
   {
      assert(!empty());
      
      tree_node *node = root;
      
      while(!node->is_leaf()) {
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
      
      return ret;
   }
   
   void splice(simple_linear_pqueue<T>& other)
   {
      assert(!other.empty());
      assert(other.size() > 0);
      
      for(size_t i(0); i < range; ++i) {
         unsafe_linear_queue_count<T>& q(other.get_queue(i));
         
         if(!q.empty())
            push_queue(q, i);
      }
   }

	class const_iterator
	{
	private:
		
		const leaves_vector *leaves;
		bool is_end;
		size_t cur_pos;
		bool started_cur;
		typename C::const_iterator sub;
		
		inline void advance(void)
		{
			if(started_cur) {
				//printf("Next sub iterator\n");
				++sub;
			} else {
				//printf("Get iterator from leaf %d\n", cur_pos);
				sub = (*leaves)[cur_pos]->bin.begin();
				started_cur = true;
			}

			if(sub == (*leaves)[cur_pos]->bin.end()) {
				cur_pos++;
				started_cur = false;
				//printf("Reached end ... to %d\n", cur_pos);
				if(cur_pos >= leaves->size()) {
					is_end = true;
					return;
				} else {
					//printf("Call ++ again...\n");
					this->operator++(); // call it again
				}
			}
		}
		
	public:
		
		inline T operator*(void) { return *sub; }
		
		inline void operator++(void)
		{
			if(is_end)
				return;
			
			advance();
		}
		
		inline bool operator==(const const_iterator& it) const
		{
			return it.is_end && is_end;
		}
		
		inline bool operator!=(const const_iterator& it) const
		{
			return !operator==(it);
		}
		
		explicit const_iterator(const leaves_vector& v):
				leaves(&v), is_end(false), cur_pos(0), started_cur(false)
		{
			advance();
		}
		
		explicit const_iterator(): is_end(true) {}
		
	};
	
	inline const_iterator begin(void) const { return const_iterator(leaves); }
	inline const_iterator end(void) const { return const_iterator(); }

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
		delete root;
   }
};

// XXX: must use C++0X

template <typename T>
struct safe_bounded_pqueue
{
   typedef bounded_pqueue<T, push_safe_linear_queue<T>, utils::atomic<size_t> > type;
};

template <typename T>
struct unsafe_bounded_pqueue
{
   typedef bounded_pqueue<T, unsafe_linear_queue<T>, size_t> type;
};

template <typename T>
struct intrusive_unsafe_double_bounded_pqueue
{
	typedef bounded_pqueue<T, intrusive_unsafe_double_queue<T>, size_t> type;
};

}

#endif

