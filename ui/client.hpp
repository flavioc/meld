
#ifndef UI_CLIENT_HPP
#define UI_CLIENT_HPP

#include <websocketpp/websocketpp.hpp>

namespace ui
{

class client
{
   public: /// XXX should be a friend of manager

      typedef websocketpp::server::connection_ptr connection_ptr;

      connection_ptr conn;
	
		bool is_alive(void) const;

      explicit client(connection_ptr);
};

}

#endif
