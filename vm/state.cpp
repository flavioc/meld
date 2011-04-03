
#include "vm/state.hpp"

using namespace vm;
using namespace db;
using namespace process;

namespace vm
{

program *state::PROGRAM = NULL;
database *state::DATABASE = NULL;
machine *state::MACHINE = NULL;

}