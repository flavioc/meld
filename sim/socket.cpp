
#include <vector>

#include "sim/socket.hpp"
#include "vm/defs.hpp"

using namespace std;
using boost::asio::ip::udp;

namespace sim
{
	
socket::socket(const string& host, const string& port):
	s(NULL), receiver(NULL)
{
	boost::asio::io_service io_service;
	
	udp::resolver resolver(io_service);
	udp::resolver::query query(udp::v4(), host.c_str(), port.c_str());
	
	receiver = new udp::endpoint(*resolver.resolve(query));
	
	s = new udp::socket(io_service);
	s->open(udp::v4());
}

void
socket::send(const message_buf data, const size_t len)
{
	std::vector<utils::byte> vec;
	
	for(size_t i(0); i < len; ++i)
		vec.push_back(data[i]);
	
	s->send_to(boost::asio::buffer(vec), *receiver);
}

message_buf
socket::receive(void)
{
	boost::array<utils::byte, MESSAGE_SIZE_BYTES> buf;
	
	udp::endpoint sender_endpoint;
	size_t len(s->receive_from(boost::asio::buffer(buf), sender_endpoint));
	
	assert(len == MESSAGE_SIZE_BYTES);
	
	message_buf msg(new utils::byte[MESSAGE_SIZE_BYTES]);
	
	for(size_t i(0); i < MESSAGE_SIZE_BYTES; ++i) {
		msg[i] = buf[i];
	}
	
	return msg;
}

void
socket::send_start_simulation(void)
{
	utils::byte *b(new utils::byte[MESSAGE_SIZE_BYTES]);
	vm::uint_val *v((vm::uint_val*)b);
	
	v[0] = 1;
	
	send(b, MESSAGE_SIZE_BYTES);
}

}