
#ifndef MEM_MEM_NODE_HPP
#define MEM_MEM_NODE_HPP

namespace mem
{
   struct chunk;

   struct mem_node {
#ifdef USE_REFCOUNT
      struct chunk *chunk;
#endif
      struct mem_node *next;
   };
#ifdef REF_COUNT
   const size_t MEM_MIN_OBJS(sizeof(mem_node)-sizeof(chunk*));
   const size_t MEM_ADD_OBJS(sizeof(struct chunk*));
#else
   const size_t MEM_MIN_OBJS(sizeof(mem_node));
   const size_t MEM_ADD_OBJS(0);
#endif

   struct free_queue {
      mem_node *head{nullptr};
      mem_node *tail{nullptr};
   };

   static inline void add_free_queue(free_queue& q, void *ptr)
   {
      mem_node *new_node((mem_node*)ptr);
      
      if(q.tail)
         q.tail->next = new_node;
      new_node->next = nullptr;
      if(!q.head)
         q.head = new_node;
   }

   static inline void init_free_queue(free_queue& q)
   {
      q.head = q.tail = nullptr;
   }

   static inline bool free_queue_empty(free_queue& q)
   {
      return q.head == nullptr;
   }

   static inline void* free_queue_pop(free_queue& q)
   {
      void *ret = q.head;
      if(q.head == q.tail)
         init_free_queue(q);
      else
         q.head = q.head->next;
      return ret;
   }
}

#endif
