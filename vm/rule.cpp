
#include "vm/rule.hpp"
#include "vm/state.hpp"
#include "vm/predicate.hpp"
#include "vm/program.hpp"
#include "vm/instr.hpp"

using namespace std;

namespace vm
{
	
void
rule::print(ostream& out, const vm::program *const prog) const
{
	out << str << endl;
   assert(theProgram);
	
	for(auto it(predicates.begin()), end(predicates.end()); it != end; it++)
	{
		const predicate_id id(*it);
		const predicate *pred(theProgram->get_predicate(id));

      assert(pred);

      out << pred->get_name() << endl;
		
	}
   if(is_persistent)
      out << "Is persistent" << endl;
   else
      out << "Not persistent" << endl;

   instrs_print(code, code_size, 0, prog, out);
}

}
