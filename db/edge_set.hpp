
#ifndef DB_EDGE_SET_HPP
#define DB_EDGE_SET_HPP

#include <tr1/unordered_set>
#include <set>

#include "vm/defs.hpp"
#include "mem/allocator.hpp"

namespace db
{

#if 0
typedef std::tr1::unordered_set<vm::node_val,
         std::hash<vm::node_val>,
         std::equal_to<vm::node_val>,
         mem::allocator<vm::node_val> > edge_set;
#else
typedef std::set<vm::node_val,
               std::less<vm::node_val>,
               mem::allocator<vm::node_val> > edge_set;
#endif
}

#endif
