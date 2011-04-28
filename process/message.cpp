
#include "process/message.hpp"

using namespace std;
using namespace process;

namespace process
{

ostream&
operator<<(ostream& cout, const message& msg)
{
   cout << "#" << "(" << *(msg.data) << " -> " << msg.id << ")";
   return cout;
}

}