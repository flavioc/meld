
#ifndef UTILS_SERIALIZATION_HPP
#define UTILS_SERIALIZATION_HPP

#include <assert.h>
#include <string.h>

#include "utils/types.hpp"

namespace utils
{

template <typename T>
void pack(void *data, const size_t size, byte *buf, const size_t buf_size, int* pos)
{
   assert(*pos + sizeof(T) * size <= buf_size);

   memcpy(buf + *pos, data, sizeof(T) * size);
   *pos = *pos + sizeof(T) * size;
}

template <typename T>
void unpack(byte *buf, const size_t buf_size, int *pos, void *data, const size_t size)
{
   assert(*pos + sizeof(T) * size <= buf_size);

   memcpy(data, buf + *pos, sizeof(T) * size);
   *pos = *pos + sizeof(T) * size;
}

}

#endif
