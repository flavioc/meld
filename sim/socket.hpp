
#ifndef SIM_SOCKET_HPP
#define SIM_SOCKET_HPP

#include <boost/asio.hpp>
#include <string>

#include "utils/types.hpp"

#define MESSAGE_SIZE_BYTES 32

namespace sim
{
	
typedef utils::byte* message_buf;

class socket
{
private:
	
	boost::asio::ip::udp::socket *s;
	boost::asio::ip::udp::endpoint *receiver;
	
public:
	
	void send(const message_buf, const size_t);
	message_buf receive(void);
	
	void send_start_simulation(void);
	
	explicit socket(const std::string&, const std::string&);
	
	~socket(void) {
		assert(s);
		delete s;
		assert(receiver);
		delete receiver;
	}
};

}

#endif

