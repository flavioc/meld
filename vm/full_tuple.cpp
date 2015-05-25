#include <assert.h>

#include "vm/full_tuple.hpp"

using namespace std;
using namespace utils;

namespace vm {

void full_tuple::pack(byte *buf, const size_t buf_size, int *pos) const {
   utils::pack<derivation_direction>((void *)&dir, 1, buf, buf_size, pos);
   utils::pack<depth_t>((void *)&depth, 1, buf, buf_size, pos);

   data->pack(get_predicate(), buf, buf_size, pos);
}

full_tuple *full_tuple::unpack(vm::predicate *pred, byte *buf,
                               const size_t buf_size, int *pos,
                               vm::program *prog, mem::node_allocator *alloc) {
   derivation_direction dir;
   depth_t depth;

   utils::unpack<derivation_direction>(buf, buf_size, pos, &dir, 1);
   utils::unpack<vm::depth_t>(buf, buf_size, pos, &depth, 1);

   vm::tuple *tpl(vm::tuple::unpack(buf, buf_size, pos, prog, alloc));

   return new full_tuple(tpl, pred, dir, depth);
}

full_tuple::~full_tuple(void) {
   // full_tuple is not allowed to delete tuples
}

void full_tuple::print(ostream &cout) const {
   if (dir == NEGATIVE_DERIVATION) cout << "-";
   data->print(cout, get_predicate());
}

ostream &operator<<(ostream &cout, const full_tuple &tuple) {
   tuple.print(cout);
   return cout;
}
}
