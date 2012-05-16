
#include "sched/thread/prio_queue.hpp"

namespace sched
{
	
void
prio_queue::add(db::node *node, const int prio)
{
	prio_obj obj(node, prio);
	
	queue.push(obj);
}

prio_obj
prio_queue::remove(void)
{
	return queue.pop();
}

void
prio_queue::save(prio_obj& p)
{
	saved.push_back(p);
}

prio_queue::prio_iterator
prio_queue::delete_saved(prio_iterator p)
{
	return saved.erase(p);
}

}
