
#ifndef VM_IMPORT_HPP
#define VM_IMPORT_HPP

#include <string>
#include <ostream>

namespace vm
{

class import
{
   private:

      std::string imp;
      std::string as;
      std::string file;

  public:

      inline void print(std::ostream& out) const
      {
         out << imp << " as " << as << " from " << file;
      }

      explicit import(const std::string& _imp, const std::string& _as,
            const std::string& _file):
         imp(_imp), as(_as), file(_file)
      {
      }
};

inline std::ostream&
operator<<(std::ostream& out, const import& imp)
{
   imp.print(out);
   return out;
}


}

#endif
