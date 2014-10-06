
#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <dlfcn.h>

#include "vm/program.hpp"
#include "db/tuple.hpp"
#include "vm/instr.hpp"
#include "db/database.hpp"
#include "utils/types.hpp"
#include "vm/state.hpp"
#include "vm/reader.hpp"
#include "vm/external.hpp"
#include "version.hpp"
#ifdef USE_UI
#include "ui/macros.hpp"
#endif

using namespace std;
using namespace db;
using namespace vm;
using namespace vm::instr;
using namespace process;
using namespace utils;

namespace vm {

all* All;     // global variable that holds pointer to vm
   // all structure.  Set by process/machine.cpp
   // in constructor.
program* theProgram;

// most integers in the byte-code have 4 bytes
static_assert(sizeof(uint_val) == 4, "uint_val must be 4 bytes long.");

static inline bool
predicate_sorter(const predicate* p1, const predicate* p2)
{
	return p1->get_name() < p2->get_name();
}

static inline size_t
next_multiple_of_uint(const size_t v)
{
   size_t ret(v / BITMAP_BITS);
   if(v % BITMAP_BITS > 0)
      ret++;
   return ret;
}

static inline ptr_val 
get_function_pointer(char *lib_path, char* func_name)
{
   void *handle = dlopen(lib_path, RTLD_LAZY);

   if(!handle) {
      cerr<<"Cannot Open Library : "<<dlerror()<<endl;
      return 0;
   }

   typedef void (*func_t)();

   //reset errors
   dlerror();

   func_t func = (func_t)dlsym(handle, func_name);
   const char *dlsym_error = dlerror();

   if(dlsym_error) {
      dlclose(handle);
      return 0;
   }

   return (ptr_val)func;
}

program::program(const string& _filename):
   filename(_filename),
   init(NULL)
{
   code_reader read(filename);

   // read magic
   uint32_t magic1, magic2;
   read.read_type<uint32_t>(&magic1);
   read.read_type<uint32_t>(&magic2);
   if(magic1 != MAGIC1 || magic2 != MAGIC2)
      throw load_file_error(filename, "not a meld byte code file");

   // read version
   read.read_type<uint32_t>(&major_version);
   read.read_type<uint32_t>(&minor_version);
   if(!VERSION_AT_LEAST(0, 11))
      throw load_file_error(filename, string("unsupported byte code version"));

   if(VERSION_AT_LEAST(0, 12))
      throw load_file_error(filename, string("unsupported byte code version"));

   // read number of predicates
   byte num_preds;
	read.read_type<byte>(&num_preds);
   
   const size_t num_predicates = (size_t)num_preds;
   num_predicates_uint = next_multiple_of_uint(num_predicates);
   
   predicates.resize(num_predicates);
   sorted_predicates.resize(num_predicates);
   code_size.resize(num_predicates);
   code.resize(num_predicates);

   // skip nodes
   uint_val num_nodes;
	read.read_type<uint_val>(&num_nodes);

#ifdef USE_REAL_NODES
   node_references = new vector<byte_code>[num_nodes];
#endif

	read.seek(num_nodes * database::node_size);

   // read number of types
   byte ntypes;
   read.read_type<byte>(&ntypes);
   types.resize((size_t)ntypes);

   for(size_t i(0); i < num_types(); ++i) {
      types[i] = read_type_from_reader(read);
   }

   // read imported/exported predicates
   uint32_t number_imported_predicates;

   read.read_type<uint32_t>(&number_imported_predicates);

   for(uint32_t i(0); i < number_imported_predicates; ++i) {
      uint32_t size;
      read.read_type<uint32_t>(&size);

      char buf_imp[size + 1];
      read.read_any(buf_imp, size);
      buf_imp[size] = '\0';

      read.read_type<uint32_t>(&size);
      char buf_as[size + 1];
      read.read_any(buf_as, size);
      buf_as[size] = '\0';

      read.read_type<uint32_t>(&size);
      char buf_file[size + 1];
      read.read_any(buf_file, size);
      buf_file[size] = '\0';

      cout << "import " << buf_imp << " as " << buf_as << " from " << buf_file << endl;

      imported_predicates.push_back(new import(buf_imp, buf_as, buf_file));
   }
   assert(imported_predicates.size() == number_imported_predicates);

   uint32_t number_exported_predicates;

   read.read_type<uint32_t>(&number_exported_predicates);

   for(uint32_t i(0); i < number_exported_predicates; ++i) {
      uint32_t str_size;
      read.read_type<uint32_t>(&str_size);
      char buf[str_size + 1];
      read.read_any(buf, str_size);
      buf[str_size] = '\0';
      exported_predicates.push_back(string(buf));
   }
   assert(exported_predicates.size() == number_exported_predicates);

	// get number of args needed
	byte n_args;

	read.read_type<byte>(&n_args);
	num_args = (size_t)n_args;

   // get rule information
   uint32_t n_rules;

   read.read_type<uint32_t>(&n_rules);

   number_rules = n_rules;
   number_rules_uint = next_multiple_of_uint(number_rules);

   for(size_t i(0); i < n_rules; ++i) {
      // read rule string length
      uint32_t rule_len;

      read.read_type<uint32_t>(&rule_len);

      assert(rule_len > 0);

      char str[rule_len + 1];

      read.read_any(str, rule_len);

      str[rule_len] = '\0';

      rules.push_back(new rule((rule_id)i, string(str), num_predicates_uint));
   }

	// read string constants
	uint32_t num_strings;
	read.read_type<uint32_t>(&num_strings);
	
	default_strings.reserve(num_strings);
	
	for(uint32_t i(0); i < num_strings; ++i) {
		uint32_t length;
		
		read.read_type<uint32_t>(&length);
		
		char str[length + 1];
		read.read_any(str, length);
		str[length] = '\0';
		default_strings.push_back(runtime::rstring::make_default_string(str));
	}
	
	// read constants code
	uint32_t num_constants;
	read.read_type<uint32_t>(&num_constants);
	
	// read constant types
	const_types.resize(num_constants);
	
	for(uint_val i(0); i < num_constants; ++i) {
      const_types[i] = read_type_id_from_reader(read, types);
	}
	
	// read constants code
	read.read_type<code_size_t>(&const_code_size);
	
	const_code = new byte_code_el[const_code_size];
	read.read_any(const_code, const_code_size);
   read_node_references(const_code, read);

   MAX_STRAT_LEVEL = 0;

   // get function code
   uint32_t n_functions;

   read.read_type<uint32_t>(&n_functions);

   functions.resize(n_functions);

   for(uint32_t i(0); i < n_functions; ++i) {
      code_size_t fun_size;

      read.read_type<code_size_t>(&fun_size);
      byte_code fun_code(new byte_code_el[fun_size]);
      read.read_any(fun_code, fun_size);

      functions[i] = new vm::function(fun_code, fun_size);
      read_node_references(fun_code, read);
   }

   // get external functions definitions
   uint32_t n_externs;

   read.read_type<uint32_t>(&n_externs);

   for(uint32_t i(0); i < n_externs; ++i) {
      uint32_t extern_id;

      read.read_type<uint32_t>(&extern_id);
      char extern_name[256];

      read.read_any(extern_name, sizeof(extern_name));

      char skip_filename[1024];

      read.read_any(skip_filename, sizeof(skip_filename));

      ptr_val skip_ptr;

      read.read_type<ptr_val>(&skip_ptr);

      skip_ptr = get_function_pointer(skip_filename,extern_name);
      uint32_t num_args;

      read.read_type<uint32_t>(&num_args);

      type *ret_type = read_type_id_from_reader(read, types);

      if(num_args) {
         type *arg_type[num_args];
         for(uint32_t j(0); j != num_args; ++j) {
            arg_type[j] = read_type_id_from_reader(read, types);
         }

         register_custom_external_function((external_function_ptr)skip_ptr, num_args, ret_type, arg_type, extern_name);
      } else
         register_custom_external_function((external_function_ptr)skip_ptr, 0, ret_type, NULL, extern_name);
   }

   // read predicate information
   total_arguments = 0;

   for(size_t i(0); i < num_predicates; ++i) {
      code_size_t size;

      sorted_predicates[i] = predicates[i] =
         predicate::make_predicate_from_reader(read, &size, (predicate_id)i, major_version, minor_version, types, number_rules_uint);
      code_size[i] = size;

      predicate *pred(predicates[i]);

      MAX_STRAT_LEVEL = max(pred->get_strat_level() + 1, MAX_STRAT_LEVEL);

      if(pred->is_route_pred())
         route_predicates.push_back(pred);

      pred->set_argument_position(total_arguments);
      total_arguments += pred->num_fields();
   }

   // create 'sorted_predicates' from 'predicates'
   sort(sorted_predicates.begin(), sorted_predicates.end(), predicate_sorter);

   safe = true;
   for(size_t i(0); i < num_predicates; ++i) {
      predicates[i]->cache_info(this);
      if(predicates[i]->is_aggregate_pred() && predicates[i]->is_unsafe_agg()) {
         safe = false;
		}
   }

	// get global priority information
	byte global_info;
	
	read.read_type<byte>(&global_info);

   initial_priority = 0.0;
   priority_static = false;

   is_data_file = false;
	
   switch(global_info) {
      case 0x01: { // priority by predicate
         cerr << "Not supported anymore" << endl;
         assert(false);
      }
      break;
      case 0x02: { // normal priority
         byte type(0x0);
         byte asc_desc;

         read.read_type<byte>(&type);
         assert(type == 0x01);

         read.read_type<byte>(&asc_desc);
         if(asc_desc & 0x01)
            priority_order = PRIORITY_ASC;
         else
            priority_order = PRIORITY_DESC;
         priority_static = (asc_desc & 0x02) ? true : false;

         read.read_type<float_val>(&initial_priority);
      }
      break;
      case 0x03: { // data file
         is_data_file = true;
      }
      break;
      default:
         priority_order = PRIORITY_DESC;
      break;
   }
   
   // read predicate code
   for(size_t i(0); i < num_predicates; ++i) {
      const size_t size = code_size[i];
      code[i] = new byte_code_el[size];
      
		read.read_any(code[i], size);
      read_node_references(code[i], read);
   }

   // read rules code
	uint32_t num_rules_code;
	read.read_type<uint32_t>(&num_rules_code);

   assert(num_rules_code == number_rules);

   for(size_t i(0); i < num_rules_code; ++i) {
      code_size_t code_size;
      byte_code code;
      read.read_type<code_size_t>(&code_size);

      code = new byte_code_el[code_size];
      read.read_any(code, code_size);

      rules[i]->set_bytecode(code_size, code);

      read_node_references(code, read);

      byte is_persistent(0x0);

      read.read_type<byte>(&is_persistent);

      if(is_persistent == 0x1)
         rules[i]->set_as_persistent();

      uint32_t num_preds;

      read.read_type<uint32_t>(&num_preds);

      assert(num_preds < 10);

      for(size_t j(0); j < num_preds; ++j) {
         predicate_id id;
         read.read_type<predicate_id>(&id);
         predicate *pred(predicates[id]);

         pred->add_affected_rule(i);
         rules[i]->add_predicate(pred);
      }
   }

   data_rule = NULL;
}

program::~program(void)
{
   for(size_t i(0); i < num_types(); ++i) {
      delete types[i];
   }
   for(size_t i(0); i < num_predicates(); ++i) {
      predicates[i]->destroy(number_rules_uint);
      delete predicates[i];
      delete []code[i];
   }
	for(size_t i(0); i < num_rules(); ++i) {
      rules[i]->destroy(num_predicates_uint);
		delete rules[i];
	}
   if(data_rule != NULL)
      delete data_rule;
   for(size_t i(0); i < functions.size(); ++i) {
      delete functions[i];
   }
	delete []const_code;
   for(size_t i(0); i < imported_predicates.size(); ++i) {
      delete imported_predicates[i];
   }
   MAX_STRAT_LEVEL = 0;
#ifdef USE_REAL_NODES
   delete []node_references;
#endif
}

void
program::read_node_references(byte_code code, code_reader& read)
{
   uint_val size_nodes;
   read.read_type<uint_val>(&size_nodes);
   uint_val pos[size_nodes];
   read.read_type<uint_val>(pos, size_nodes);
#ifdef USE_REAL_NODES
   for(uint_val i(0); i < size_nodes; ++i) {
      byte_code p(code + pos[i]);
      const node_val n(pcounter_node(p));
      node_references[n].push_back(p);
   }
#else
   (void)code;
#endif
}

#ifdef USE_REAL_NODES
void
program::fix_node_address(db::node *n)
{
   vector<byte_code>& vec(node_references[n->get_id()]);
   for(vector<byte_code>::iterator it(vec.begin()), end(vec.end()); it != end; ++it)
      pcounter_set_node(*it, (node_val)n);
}
#endif

predicate*
program::get_predicate(const predicate_id& id) const
{
   if((size_t)id >= num_predicates()) {
      return NULL;
   }
   
   return predicates[id];
}

predicate*
program::get_route_predicate(const size_t& i) const
{
   assert(i < num_route_predicates());
   
   return route_predicates[i];
}

void
program::print_predicate_code(ostream& out, predicate* p) const
{
   out << "PROCESS " << p->get_name()
      << " (" << code_size[p->get_id()] << "):" << endl;
   instrs_print(code[p->get_id()], code_size[p->get_id()], 0, this, out);
   out << "END PROCESS;" << endl;
}

void
program::print_bytecode(ostream& out) const
{
   out << "VERSION " << major_version << "." << minor_version << endl << endl;

	out << "CONST CODE" << endl;
	
	instrs_print(const_code, const_code_size, 0, this, out);

   out << endl;

   out << "FUNCTION CODE" << endl;
   for(size_t i(0); i < functions.size(); ++i) {
      out << "FUNCTION " << i << endl;
      instrs_print(functions[i]->get_bytecode(), functions[i]->get_bytecode_size(), 0, this, out);
      out << endl;
   }

   out << endl;
   out << "PREDICATE CODE" << endl;
	
   for(size_t i = 0; i < num_predicates(); ++i) {
      predicate_id id = (predicate_id)i;
      
      if(i != 0)
         out << endl;
         
      print_predicate_code(out, get_predicate(id));
   }

   out << "RULES CODE" << endl;

   for(size_t i(0); i < number_rules; ++i) {
      out << endl;
      out << "RULE " << i << endl;
		rules[i]->print(out, this);
   }
}

void
program::print_bytecode_by_predicate(ostream& out, const string& name) const
{
	predicate *p(get_predicate_by_name(name));
	
	if(p == NULL) {
		cerr << "Predicate " << name << " not found." << endl;
		return;
	}
   print_predicate_code(out, get_predicate_by_name(name));
}

void
program::print_program(ostream& out) const
{
   for(size_t i(0); i < number_rules; ++i) {
      out << rules[i]->get_string() << endl;
   }
}

void
program::print_rules(ostream& out) const
{
   for(size_t i(0); i < number_rules; ++i) {
      out << endl;
      out << "RULE " << i << endl;
      out << rules[i]->get_string() << endl;
   }
}

void
program::print_predicates(ostream& cout) const
{
   if(is_safe())
      cout << ">> Safe program" << endl;
   else
      cout << ">> Unsafe program" << endl;
   cout << ">> Priorities: " << (priority_order == PRIORITY_ASC ? "ascending" : "descending") << " ";
   cout << "initial: " << initial_priority << endl;
   if(priority_static)
      cout << ">> No work stealing" << endl;
   if(is_data())
      cout << ">> Data file" << endl;
   cout << ">> Predicates:" << endl;
   for(size_t i(0); i < num_predicates(); ++i) {
      cout << predicates[i] << " " << *predicates[i] << endl;
   }
   cout << ">> Imported Predicates:" << endl;
   for(size_t i(0); i < imported_predicates.size(); i++) {
      cout << *imported_predicates[i] << endl;
   }
   cout << ">> Exported Predicates:" << endl;
   for(size_t i(0); i < exported_predicates.size(); i++) {
      cout << exported_predicates[i] << endl;
   }
}

#ifdef USE_UI
using namespace json_spirit;

Value
program::dump_json(void) const
{
	Array preds_json;

	for(size_t i(0); i < num_predicates(); ++i) {
		Object obj;
		predicate *pred(get_predicate((predicate_id)i));

		UI_ADD_FIELD(obj, "name", pred->get_name());

		Array field_types;

		for(size_t j(0); j < pred->num_fields(); ++j) {
         UI_ADD_ELEM(field_types, pred->get_field_type(j)->string());
		}
		UI_ADD_FIELD(obj, "fields", field_types);

		UI_ADD_FIELD(obj, "route",
				pred->is_route_pred() ? UI_YES : UI_NIL);
		UI_ADD_FIELD(obj, "reverse_route",
				pred->is_reverse_route_pred() ? UI_YES : UI_NIL);
		UI_ADD_FIELD(obj, "linear",
			pred->is_linear_pred() ? UI_YES : UI_NIL);

		UI_ADD_ELEM(preds_json, obj);
	}

	return preds_json;
}
#endif

predicate*
program::get_predicate_by_name(const string& name) const
{
   for(size_t i(0); i < num_predicates(); ++i) {
      predicate *pred(get_predicate((predicate_id)i));
      
      if(pred->get_name() == name)
         return pred;
   }
   
   return NULL;
}

predicate*
program::get_init_predicate(void) const
{
   if(init == NULL) {
      init = get_predicate(INIT_PREDICATE_ID);
      if(init->get_name() != "_init") {
         cerr << "_init program should be predicate #" << (int)INIT_PREDICATE_ID << endl;
         init = get_predicate_by_name("_init");
      }
      assert(init->get_name() == "_init");
   }

   assert(init != NULL);
      
   return init;
}

predicate*
program::get_edge_predicate(void) const
{
   return get_predicate_by_name("edge");
}

bool
program::add_data_file(program& other)
{
   if(num_predicates() < other.num_predicates()) {
      return false;
   }

   for(size_t i(0); i < other.num_predicates(); ++i) {
      predicate *mine(predicates[i]);
      predicate *oth(other.get_predicate(i));
      if(*mine != *oth) {
         cerr << "Predicates " << *mine << " and " << *oth << " are different" << endl;
         return false;
      }
   }

   return false;
#if 0
   XXXX
   assert(rules.size() > 0);
   assert(other.rules.size() > 0);

   data_rule = other.rules[0];
   other.rules.erase(other.rules.begin());
   other.number_rules--;

   vm::rule *init_rule(rules[0]);
   byte_code init_code(init_rule->get_bytecode());
   init_code += init_rule->get_codesize();

   init_code -= (RETURN_BASE + NEXT_BASE + RETURN_DERIVED_BASE + MOVE_BASE + ptr_size);
   assert(fetch(init_code) == MOVE_INSTR);
   assert(val_is_ptr(move_from(init_code)));
   assert(val_is_reg(move_to(init_code)));
   *move_to_ptr(init_code) = VAL_PCOUNTER;
   *((ptr_val *)(init_code + MOVE_BASE)) = (ptr_val)data_rule->get_bytecode();

   //instrs_print(init_rule->get_bytecode(), init_rule->get_codesize(), 0, this, cout);
#endif
   return true;
}

}
