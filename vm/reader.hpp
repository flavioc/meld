
#ifndef VM_READER_HPP
#define VM_READER_HPP

// reads byte code files

#include <fstream>
#include <string>

#include "vm/defs.hpp"
#include "utils/types.hpp"

namespace vm
{

class load_file_error : public std::runtime_error {
 public:
    explicit load_file_error(const std::string& filename, const std::string& reason) :
         std::runtime_error(std::string("unable to load byte-code file ") + filename + ": " + reason)
    {}
};

class code_reader
{
   private:

      std::ifstream fp;
      size_t position;

   public:

      template <typename T>
      inline void read_type(T *out, const size_t n = 1)
      {
         read((utils::byte*)out, sizeof(T) * n);
      }

      inline void read(utils::byte *out, const size_t size)
      {
         fp.read((char *)out, size);
         position += size;
      }

      template <typename T>
      inline void read_any(T *out, const size_t size)
      {
         read((utils::byte*)out, size);
      }

      inline void seek(const size_t size)
      {
         fp.seekg(size, std::ios_base::cur);
         position += size;
      }

      explicit code_reader(const std::string& file_name):
            fp(file_name.c_str(), std::ios::in | std::ios::binary)
      {
         if(!fp.is_open())
            throw load_file_error(file_name, std::string("could not open file"));
      }
};

}

#endif
