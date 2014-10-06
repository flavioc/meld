
#ifndef UTILS_RANDOM_HPP
#define UTILS_RANDOM_HPP

#include <random>
#include <vector>
#include <algorithm>

namespace utils
{
   
class randgen
{
private:
   
   std::mt19937 state;
   
public:
   
   unsigned operator()(unsigned i) {
      std::uniform_int_distribution<unsigned> rng(0, i - 1);
      return rng(state);
   }
   
   explicit randgen(const size_t seed = 5489U): state(seed) {}
};

template <class T>
void
shuffle_vector(T& vec, randgen& gen)
{
	std::random_shuffle(vec.begin(), vec.end(), gen);
}
   
}
#endif
