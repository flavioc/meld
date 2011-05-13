
#ifndef UTILS_ATOMIC_HPP
#define UTILS_ATOMIC_HPP

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
   inline T& operator++ () {
      return add_and_fetch(1);
   }
   
   // postfix version of atomic++
   inline T operator++ (int) {
      return fetch_and_add(1);
   }
   
   // prefix version of --atomic
   inline T& operator-- () {
      return sub_and_fetch(1);
   }
   
   // postfix version of atomic--
   inline T operator-- (int) {
      return fetch_and_sub(1);
   }
   
   operator T() const { return elem; }
   
   explicit atomic(const T _elem): elem(_elem) {}
   
   ~atomic(void) {}
};

}

#endif
