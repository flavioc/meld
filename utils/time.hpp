
#ifndef UTILS_TIME_HPP
#define UTILS_TIME_HPP

#include <ostream>
#include <boost/date_time.hpp>

namespace utils
{

class execution_time
{
private:
   
   boost::posix_time::time_duration dur;
   boost::posix_time::ptime before;

public:
   
   class scope
   {
   private:
      
      execution_time& t;
      
   public:
      
      explicit scope(execution_time& _t): t(_t) { t.start(); }
      
      ~scope(void) { t.stop(); }
   };
   
   void start(void) { before = boost::posix_time::microsec_clock::local_time(); }
   
   void stop(void)
   {
      dur += (boost::posix_time::microsec_clock::local_time() - before);
   }
   
   inline size_t milliseconds(void) const { return dur.total_milliseconds(); }
   
   void print(std::ostream& cout) const { cout << milliseconds() << "ms"; }
   
   explicit execution_time(void) {}
   
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
   gettimeofday(&tv, NULL);

   return tv.tv_sec * (unix_timestamp)1000 +
      tv.tv_usec/1000;
}

}

#endif
