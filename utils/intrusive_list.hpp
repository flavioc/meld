
#ifndef UTILS_INTRUSIVE_LIST_HPP
#define UTILS_INTRUSIVE_LIST_HPP

#include <iostream>

namespace utils
{

#define DECLARE_LIST_INTRUSIVE(TYPE)      \
   void *__intrusive_next;                \
   void *__intrusive_prev

template <class T>
class identity_intrusive_next
{
   public:

      inline T *operator()(T *x)
      {
         return (T*)(x->__intrusive_next);
      }

      inline void set(T *x, T *val)
      {
         x->__intrusive_next = val;
      }
};

template <class T>
class identity_intrusive_prev
{
   public:

      inline T *operator()(T *x)
      {
         return (T*)(x->__intrusive_prev);
      }

      inline void set(T *x, T *val)
      {
         x->__intrusive_prev = val;
      }
};

template <class T>
class indirect_intrusive_next
{
   public:

      inline T *operator()(T *x)
      {
         return (T*)(x->data->__intrusive_next);
      }

      inline void set(T *x, T *val)
      {
         x->data->__intrusive_next = val;
      }
};

template <class T>
class indirect_intrusive_prev
{
   public:

      inline T *operator()(T *x)
      {
         return (T*)(x->data->__intrusive_prev);
      }

      inline void set(T *x, T *val)
      {
         x->data->__intrusive_prev = val;
      }
};

template <class T, class Next = identity_intrusive_next<T>, class Prev = identity_intrusive_prev<T> >
struct intrusive_list
{
   private:

      T *head;
      T *tail;
      size_t size;

   public:

      struct iterator
      {
         public:
            T *current;

         public:

            inline T* operator*(void) const
            {
               return current;
            }

            inline bool operator==(const iterator& it) const
            {
               return current == it.current;
            }
            inline bool operator!=(const iterator& it) const { return !operator==(it); }

            inline iterator& operator++(void)
            {
               current = Next()(current);
               return *this;
            }

            inline iterator operator++(int)
            {
               current = Next()(current);
               return *this;
            }

            explicit iterator(T* cur): current(cur) {}

            // end iterator
            explicit iterator(void): current(NULL) {}
      };

      typedef iterator const_iterator;

      inline bool empty(void) const { assert((head == NULL && size == 0) || (head != NULL && size > 0)); return head == NULL; }
      inline size_t get_size(void) const { return size; }

      inline void assertl(void)
      {
#ifndef NDEBUG
         if(head == NULL) {
            assert(size == 0);
            assert(tail == NULL);
         } else {
            assert(size > 0);
            T* p(head);
            T* prev(NULL);
            assert(tail);
            while(p) {
               assert(Prev()(p) == prev);
               prev = p;
               p = Next()(p);
               assert(p != prev);
               assert(Next()(prev) == p);
            }
            assert(prev == tail);
         }
#endif
      }

      inline iterator erase(iterator& it)
      {
         T* obj(*it);
         //std::cout << "erase " << this << " " << *obj << std::endl;
         iterator it2(Next()(it.current));
         assert(size > 0);
         if(obj == head) {
            head = Next()(head);
            assert(Prev()(obj) == NULL);
            if(obj == tail) {
               tail = NULL;
               assert(size == 1);
               assert(head == NULL);
               assert(Next()(obj) == NULL);
            } else {
               Prev().set(head, NULL);
               assert(Next()(tail) == NULL);
               assert(Prev()(head) == NULL);
            }
         } else {
            assert(size > 1);
            T* prev(Prev()(obj));
            T* next(Next()(obj));

            assert(prev);
            Next().set(prev, next);
            if(next)
               Prev().set(next, prev);
            else {
               tail = prev;
               assert(next == NULL);
            }
            assert(Next()(tail) == NULL);
            assert(Prev()(head) == NULL);
         }
         size--;
         //assertl();
         return it2;
      }

      inline iterator begin(void) { return iterator(head); }
      inline iterator end(void) { return iterator(); }
      inline const iterator begin(void) const { return iterator(head); }
      inline const iterator end(void) const { return iterator(); }

      inline void push_front(T *n)
      {
         if(head == NULL) {
            assert(size == 0);
            assert(tail == NULL);
            head = tail = n;
            Next().set(n, NULL);
         } else {
            assert(head && tail);
            assert(size > 0);
            Prev().set(head, n);
            Next().set(n, head);
            head = n;
         }
         Prev().set(n, NULL);
         size++;
         //assertl();
         assert(n == head);
         assert(Next()(tail) == NULL);
         assert(Prev()(head) == NULL);
      }

      inline T* pop_front(void)
      {
         if(head == NULL)
            return NULL;

         T *ret(head);
         head = Next()(head);
         if(head)
            Prev().set(head, NULL);
         else
            tail = NULL;
         size--;
         return ret;
      }

      inline void push_back(T *n)
      {
         //assertl();
         if(head == NULL) {
            assert(size == 0);
            assert(tail == NULL);
            head = tail = n;
            Prev().set(n, NULL);
         } else {
            assert(tail);
            assert(size > 0);
            Next().set(tail, n);
            Prev().set(n, tail);
            assert(Prev()(tail) != tail);
            tail = n;
            assert(tail != head);
         }
         Next().set(tail, NULL);
         size++;
         assert(n == tail);
         assert(Next()(tail) == NULL);
         assert(Prev()(head) == NULL);
         //assertl();
      }

      inline void splice_front(intrusive_list& ls)
      {
         //assertl();
         if(tail == NULL) {
            assert(head == NULL);
            head = ls.head;
            tail = ls.tail;
            assert(!tail || Next()(tail) == NULL);
            assert(!head || Prev()(head) == NULL);
         } else {
            assert(head);
            Prev().set(head, ls.tail);
            if(ls.tail) {
               Next().set(ls.tail, head);
               assert(ls.head);
               head = ls.head;
            } else {
               assert(ls.head == NULL);
               assert(Prev()(head) == NULL);
            }
            assert(Next()(tail) == NULL);
            assert(Prev()(head) == NULL);
         }
         size += ls.size;
         ls.clear();
         //assertl();
      }

      inline void splice_back(intrusive_list& ls)
      {
         //std::cout << "splice " << this << " " << &ls << std::endl;
         //ls.assertl();
         //assertl();
         if(head == NULL) {
            assert(tail == NULL);
            head = ls.head;
            tail = ls.tail;
            assert(!tail || Next()(tail) == NULL);
            assert(!head || Prev()(head) == NULL);
         } else {
            assert(tail);
            Next().set(tail, ls.head);
            if(ls.head) {
               Prev().set(ls.head, tail);
               assert(ls.tail);
               tail = ls.tail;
            } else {
               assert(ls.tail == NULL);
               assert(Next()(tail) == NULL);
            }
            assert(Next()(tail) == NULL);
            assert(Prev()(head) == NULL);
         }
         size += ls.size;
         ls.clear();
         //assertl();
      }

      inline void dump(std::ostream& out, const vm::predicate *pred) const
      {
         for(T *tpl(head); tpl != NULL; tpl = Next()(tpl)) {
            out << "\t"; tpl->print(out, pred); out << "\n";
         }
      }

      inline void clear(void)
      {
         size = 0;
         head = NULL;
         tail = NULL;
         //assertl();
      }

      explicit intrusive_list(void):
         head(NULL), tail(NULL), size(0)
      {
         //assertl();
      }
};

}

#endif
