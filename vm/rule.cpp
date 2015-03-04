
#include "vm/rule.hpp"
#include "vm/state.hpp"
#include "vm/predicate.hpp"
#include "vm/program.hpp"
#include "vm/instr.hpp"

using namespace std;

namespace vm {

void rule::print(ostream &out, const vm::program *const prog) const {
   out << str << endl;
   assert(theProgram);

   for (auto id : predicates) {
      const predicate *pred(theProgram->get_predicate(id));

      assert(pred);

      out << pred->get_name() << endl;
   }

   instrs_print(code, code_size, 0, prog, out);
}
}
