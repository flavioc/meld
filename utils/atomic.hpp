
#ifndef UTILS_ATOMIC_HPP
#define UTILS_ATOMIC_HPP

#include <cstdlib>

namespace utils
{
   
template <class T>
class atomic
{
private:
   
   volatile T elem;
   
   // adds 'plus' and returns the old value
   inline T fetch_and_add(const T plus) { return __sync_fetch_and_add(&elem, plus); }
   
   // subtracts 'sub' and returns the old value
   inline T fetch_and_sub(const T sub) { return __sync_fetch_and_sub(&elem, sub); }
   
   // adds 'plus' and returns the result
   inline T add_and_fetch(const T plus) { return __sync_add_and_fetch(&elem, plus); }
   
   // subtracts 'sub' and returns the result
   inline T sub_and_fetch(const T sub) { return __sync_sub_and_fetch(&elem, sub); }
   
public:
   
   // prefix version of ++atomic
   inline T operator++ () {
      return add_and_fetch(1);
   }
   
   // postfix version of atomic++
   inline T operator++ (int) {
      return fetch_and_add(1);
   }
   
   // prefix version of --atomic
   inline T operator-- () {
      return sub_and_fetch(1);
   }
   
   // postfix version of atomic--
   inline T operator-- (int) {
      return fetch_and_sub(1);
   }
   
   // +=
   atomic & operator+=(const T val) {
      fetch_and_add(val);
      return *this;
   }
   
   atomic& operator-=(const T val) {
      fetch_and_sub(val);
      return *this;
   }
   
   inline void operator=(const T val)
   {
      elem = val;
   }
   
   operator T() const { return elem; }
   
   atomic(const T _elem): elem(_elem) {}
   
   ~atomic(void) {}
};

template <class T>
class atomic_ref
{
private:
   
   volatile T ref;
   
public:
   
   inline void operator=(volatile T val)
   {
      ref = val;
   }
   
   inline T get(void) const { return (T)ref; }
   
   inline operator T() const { return get(); }
   
   inline bool compare_test_set(T comp, T new_val)
   {
      return __sync_bool_compare_and_swap(&ref, comp, new_val);
   }
   
   inline T compare_and_set(T comp, T new_val)
   {
      return __sync_val_compare_and_swap((T*)&ref, comp, new_val);
   }
   
   atomic_ref(T _ref): ref(_ref) {}
   
   ~atomic_ref(void) {}
};

}

#endif
