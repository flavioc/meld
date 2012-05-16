
#include <string>

#include "ui/client.hpp"

using namespace std;
using namespace websocketpp;

namespace ui
{

client::client(connection_ptr _conn):
   conn(_conn)
{
}

bool
client::is_alive(void) const
{
   return true;
}

}
