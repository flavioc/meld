
#ifndef COMPILER_ASM_HPP
#define COMPILER_ASM_HPP

#include <vector>
#include <string>

#include "compiler/ast.hpp"
#include "utils/utils.hpp"
#include "vm/defs.hpp"
#include "vm/instr.hpp"

namespace compiler
{


struct value {
   enum {
      REG,
      FIELD,
      INT_CONSTANT,
      NODE_CONSTANT,
      FLOAT_CONSTANT
   } type;
   union {
      size_t reg;
      struct {
         size_t reg;
         vm::field_num field;
      } field;
      vm::tuple_field constant;
   } data;

   inline std::string str() const
   {
      switch(type) {
         case REG: return std::string("reg ") + utils::to_string(data.reg);
         case FIELD: return utils::to_string(data.field.reg) + "." + utils::to_string(data.field.field);
         case INT_CONSTANT: return utils::to_string(data.constant.int_field);
         case FLOAT_CONSTANT: return utils::to_string(data.constant.float_field);
         case NODE_CONSTANT: return utils::to_string(data.constant.node_field);
      }
   }
};

class instruction
{
   protected:

      vm::instr::instr_type code;

   public:

      virtual std::string str() const = 0;

      explicit instruction(vm::instr::instr_type _code):
         code(_code)
      {}
};

class single_instruction: public instruction
{
   private:

      std::vector<value> values;

   public:

      virtual std::string str() const
      {
         std::string ret;
         for(const value& v : values)
            ret += " " + v.str();
         return ret;
      }

      explicit single_instruction(vm::instr::instr_type code):
         instruction(code)
      {}
};

class iterate_match
{
   private:

      std::unordered_map<vm::field_num, void*> matches;

   public:

      explicit iterate_match() {}
};

class iterate_instruction: public instruction
{
   private:

      predicate_definition *def;
      size_t reg;
      // XXX match object
      std::list<instruction*> inner;

   public:

      virtual std::string str() const
      {
         std::string ret;
         switch(code) {
            case vm::instr::PERS_ITER_INSTR: ret = "PERS ITERATE"; break;
            case vm::instr::OPERS_ITER_INSTR: ret = "OPERS ITERATE"; break;
            case vm::instr::LINEAR_ITER_INSTR: ret = "LINEAR ITERATE"; break;
            case vm::instr::RLINEAR_ITER_INSTR: ret = "RLINEAR ITERATE"; break;
            case vm::instr::OLINEAR_ITER_INSTR: ret = "OLINEAR ITERATE"; break;
            case vm::instr::ORLINEAR_ITER_INSTR: ret = "ORLINEAR ITERATE"; break;
            default: abort();
         }

         ret += " OVER " + def->get_name() + " MATCHING TO reg " + utils::to_string(reg) + "\n";
         for(instruction *i : inner)
            ret += "\t" + i->str() + "\n";
         return ret;
      }

      explicit iterate_instruction(const vm::instr::instr_type typ,
            predicate_definition *_def,
            size_t _reg):
         instruction(typ), def(_def), reg(_reg)
      {}
};

class compiled_rule
{
   private:

      rule *orig_rule;
      std::vector<predicate_definition*> body_predicates;
      std::list<instruction*> instrs;

   public:

      std::string str() const
      {
         std::string ret(orig_rule->str() + "\n");
         for(instruction *instr : instrs)
            ret += instr->str() + "\n";
         return ret;
      }

      explicit compiled_rule(rule *);
};

class compiled_predicate
{
   private:

      std::list<instruction*> instrs;

   public:

      std::string str() const
      {
         std::string ret("");
         for(instruction *instr : instrs)
            ret += instr->str() + "\n";
         return ret;
      }

      explicit compiled_predicate(predicate_definition *,
            std::vector<rule*>&);
};

class compiled_program
{
   private:

      abstract_syntax_tree *ast;
      std::vector<compiled_rule*> rules;
      std::vector<compiled_predicate*> processes;

   public:

      void print(std::ostream& cout) const
      {
         cout << "PROCESSES:\n";
         for(compiled_predicate *pred : processes)
            cout << pred->str() << "\n";
         cout << "\nRULES:\n";
         for(compiled_rule *rule : rules)
            cout << rule->str() << "\n";
      }

      void compile();

      explicit compiled_program(abstract_syntax_tree *_ast):
         ast(_ast)
      {}
};

}

#endif
