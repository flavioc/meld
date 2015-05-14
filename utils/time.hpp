
#ifndef UTILS_TIME_HPP
#define UTILS_TIME_HPP

#include <sys/time.h>
#include <chrono>
#include <ostream>

namespace utils
{

class execution_time
{
private:
   
   std::chrono::milliseconds dur;
   std::chrono::time_point<std::chrono::high_resolution_clock> before;

public:
   
   class scope
   {
   private:
      
      execution_time& t;
      
   public:
      
      explicit scope(execution_time& _t): t(_t) { t.start(); }
      
      ~scope(void) { t.stop(); }
   };
   
   void start(void) { before = std::chrono::high_resolution_clock::now(); }
   
   void stop(void)
   {
      dur += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - before);
   }
   
   inline size_t milliseconds(void) const { return dur.count(); }
   
   void print(std::ostream& cout) const { cout << milliseconds() << " ms"; }

	inline bool is_zero(void) const { return milliseconds() == 0; }

   inline bool operator<(const execution_time& other) const
   {
      return dur < other.dur;
   }

   inline bool operator>(const execution_time& other) const
   {
      return dur > other.dur;
   }

   inline bool operator==(const execution_time& other) const
   {
      return dur == other.dur;
   }

	inline execution_time& operator+=(const execution_time& other) {
		dur += other.dur;
		return *this;
	}
   
   explicit execution_time(void): dur(0) {}

   execution_time(const execution_time& other): dur(other.dur) {}
   
   ~execution_time(void) {}   
};

static inline
std::ostream&
operator<<(std::ostream& cout, const execution_time& t)
{
   t.print(cout);
   return cout;
}

typedef uint64_t unix_timestamp;

inline unix_timestamp
get_timestamp(void)
{
   struct timeval tv;
   gettimeofday(&tv, nullptr);

   return tv.tv_sec * (unix_timestamp)1000 +
      tv.tv_usec/1000;
}

}

#endif
