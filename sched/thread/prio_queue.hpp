
#ifndef SCHED_THREAD_PRIO_QUEUE_HPP
#define SCHED_THREAD_PRIO_QUEUE_HPP

#include <list>

#include "queue/safe_queue.hpp"
#include "db/node.hpp"

namespace sched
{
	
class prio_obj
{
private:
	
	db::node *node;
	int prio;
	
public:
	
	int get_prio(void) const { return prio; }
	db::node *get_node(void) const { return node; }
	
	explicit prio_obj(db::node *_node, const int _prio):
		node(_node), prio(_prio)
	{
	}
	
	explicit prio_obj(void):
		node(NULL), prio(0)
	{
	}
	
	virtual ~prio_obj(void) { }
};

class prio_queue
{
	public:
	
		typedef std::list<prio_obj> prio_list;
		typedef prio_list::iterator prio_iterator;
	
	private:
		
		safe_queue<prio_obj> queue;
		prio_list saved;
		
	public:
		
		bool empty(void) const { return queue.empty(); }
		
		void add(db::node *, const int);
		
		void save(prio_obj&);
		
		prio_iterator delete_saved(prio_iterator);
		prio_iterator begin_saved(void) { return saved.begin(); }
		prio_iterator end_saved(void) { return saved.end(); }
		
		prio_obj remove(void);

		explicit prio_queue(void) { }

		virtual ~prio_queue(void) { }
};

}

#endif

