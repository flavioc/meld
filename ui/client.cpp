
#include <string>

#include "ui/client.hpp"

using namespace std;
#ifdef USE_UI
using namespace websocketpp;
#endif

namespace ui
{

#ifdef USE_UI
client::client(connection_ptr _conn):
   conn(_conn), all(NULL)
{
}

bool
client::is_alive(void) const
{
   return true;
}

#endif

}
