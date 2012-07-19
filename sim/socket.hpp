
#ifndef SIM_SOCKET_HPP
#define SIM_SOCKET_HPP

#include <boost/asio.hpp>
#include <string>

#include "utils/types.hpp"
#include "vm/defs.hpp"

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
	
	void send_start_simulation(const size_t);
	
	void send_stop_simulation(void);
	
	void send_link(const size_t, const size_t, const size_t);
	
	void send_computation_lock(const size_t, const vm::quantum_t);
	
	void send_send_message(const size_t, const size_t, const size_t);
	
	uint64_t receive_unlock(void);
	
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

