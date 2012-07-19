
#include <vector>

#include "sim/socket.hpp"
#include "vm/defs.hpp"

using namespace std;
using boost::asio::ip::udp;

#define MESSAGE_SIZE (MESSAGE_SIZE_BYTES/sizeof(uint64_t))

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

#define SEND_MESSAGE(ARG1, ARG2, ARG3, ARG4) do { 	\
uint64_t *v(new uint64_t[MESSAGE_SIZE]);	\
	v[0] = ARG1;	\
	v[1] = ARG2; 	\
	v[2] = ARG3;	\
	v[3] = ARG4;	\
	send((utils::byte*)v, MESSAGE_SIZE_BYTES);		\
} while(false)

#define SEND_MSG1(ARG)	SEND_MESSAGE(ARG, 0, 0, 0)
#define SEND_MSG2(ARG1, ARG2)	SEND_MESSAGE(ARG1, ARG2, 0, 0)
#define SEND_MSG3(ARG1, ARG2, ARG3) SEND_MESSAGE(ARG1, ARG2, ARG3, 0)
#define SEND_MSG4 SEND_MESSAGE

void
socket::send_start_simulation(const size_t nthreads)
{
	cout << "Send start simulation with " << nthreads << endl;
	SEND_MSG2(1, nthreads);
}

void
socket::send_stop_simulation(void)
{
	cout << "Send stop simulation" << endl;
	SEND_MSG1(2);
}

void
socket::send_link(const size_t t1, const size_t t2, const size_t speed)
{
	cout << "Sending link from " << t1 << " to " << t2 << " with speed " << speed << endl;
	SEND_MSG4(3, t1, t2, speed);
}

void
socket::send_computation_lock(const size_t tid, const vm::quantum_t q)
{
	cout << "Sending computation lock for thread " << tid << " with quanta " << q << endl;
	SEND_MSG3(4, tid, (uint64_t)q);
}

void
socket::send_send_message(const size_t t1, const size_t t2, const size_t sz)
{
	SEND_MSG4(6, t1, t2, sz);
}

uint64_t
socket::receive_unlock(void)
{
	message_buf buf(receive());
	uint64_t *v((uint64_t*)buf);
	
	return v[1];
}

}
