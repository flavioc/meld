
#include "sched/mpi/message.hpp"

using namespace std;
using namespace utils;
using namespace db;

namespace sched
{

#ifdef COMPILE_MPI

ostream&
operator<<(ostream& cout, const message& msg)
{
   cout << "#" << "(" << *(msg.data) << " -> " << msg.id << ")";
   return cout;
}

void
message::pack(byte *buf, const size_t buf_size, int *pos, MPI_Comm comm) const
{
   MPI_Pack((void*)&id, 1, MPI_UNSIGNED, buf, buf_size, pos, comm);
   
   data->pack(buf, buf_size, pos, comm);
}

message*
message::unpack(byte *buf, const size_t buf_size, int *pos, MPI_Comm comm, vm::program *prog)
{
   node::node_id id;
   
   MPI_Unpack(buf, buf_size, pos, &id, 1, MPI_UNSIGNED, comm);
   
   simple_tuple *stpl(simple_tuple::unpack(buf, buf_size, pos, comm, prog));
   
   return new message(id, stpl);
}

void
message_set::pack(byte *buf, const size_t buf_size, MPI_Comm comm) const
{
   int pos(0);
   unsigned short int size_msg((unsigned short int)size());
   
   MPI_Pack(&size_msg, 1, MPI_UNSIGNED_SHORT, buf, buf_size, &pos, comm);
   
   for(list_messages::const_iterator it(lst.begin()); it != lst.end(); ++it)
      (*it)->pack(buf, buf_size, &pos, comm);
}

message_set*
message_set::unpack(byte *buf, const size_t buf_size, MPI_Comm comm, vm::program *prog)
{
   message_set *ms(new message_set());
   unsigned short int size_msg;
   int pos(0);
   
   MPI_Unpack(buf, buf_size, &pos, &size_msg, 1, MPI_UNSIGNED_SHORT, comm);
   
   for(size_t i(0); i < size_msg; ++i) {
      message *msg(message::unpack(buf, buf_size, &pos, comm, prog));
      ms->add(msg);
   }
   
   return ms;
}
#endif

}
