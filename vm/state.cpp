
#include "vm/state.hpp"

using namespace vm;
using namespace db;
using namespace process;

namespace vm
{

program *state::PROGRAM = NULL;
database *state::DATABASE = NULL;
machine *state::MACHINE = NULL;
remote *state::REMOTE = NULL;
router *state::ROUTER = NULL;
size_t state::NUM_THREADS = 0;

}