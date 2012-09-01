
#ifndef UTILS_RANDOM_HPP
#define UTILS_RANDOM_HPP

#include <boost/random.hpp>
#include <vector>
#include <algorithm>

namespace utils
{
   
class randgen
{
private:
   
   boost::mt19937 state;
   
public:
   
   unsigned operator()(unsigned i) {
      boost::uniform_int<> rng(0, i - 1);
      return rng(state);
   }
   
   explicit randgen(void) {}
};

template <class T>
void
shuffle_vector(T& vec, randgen& gen)
{
	std::random_shuffle(vec.begin(), vec.end(), gen);
}
   
}
#endif
