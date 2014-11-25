
#ifndef QUEUE_ITERATORS_HPP
#define QUEUE_ITERATORS_HPP

#define QUEUE_DEFINE_LINEAR_CONST_ITERATOR_CLASS()					\
class const_iterator											\
{																	\
	private:														\
		node *cur;												\
	public:														\
		inline T operator*(void)							\
		{															\
			assert(cur != nullptr);								\
			return cur->data;									\
		}															\
																	\
		inline const_iterator&								\
		operator=(const const_iterator& it)				\
		{															\
			cur = it.cur;										\
			return *this;										\
		}															\
																	\
		inline void operator++(void)						\
		{															\
			assert(cur != nullptr);								\
			cur = (node*)cur->next;									\
		}															\
																	\
		inline bool												\
		operator==(const const_iterator& it) const	\
		{ return it.cur == cur; }							\
																	\
		explicit const_iterator(node *n): cur(n) {}	\
																	\
		explicit const_iterator(void): cur(nullptr) {}	\
};
																																																	
#define QUEUE_DEFINE_LINEAR_CONST_ITERATOR()					\
	QUEUE_DEFINE_LINEAR_CONST_ITERATOR_CLASS();					\
																\
	inline const_iterator begin(void) const						\
	{ return const_iterator(head); }							\
	inline const_iterator end(void) const						\
	{ return const_iterator(nullptr); }
				
#endif
