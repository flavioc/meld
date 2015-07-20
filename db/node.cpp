
#include <iostream>
#include <assert.h>

#include "db/node.hpp"
#include "vm/state.hpp"
#include "utils/utils.hpp"

using namespace db;
using namespace std;
using namespace vm;
using namespace utils;

namespace db {

size_t node::count_total(const predicate *pred) const {
   if (pred->is_persistent_pred()) return pers_store.count_total(pred);

   if (linear.stored_as_hash_table(pred)) {
      const hash_table *table(linear.get_hash_table(pred->get_linear_id()));
      return table->get_total_size();
   }

   const intrusive_list<vm::tuple> *ls(
       linear.get_linked_list(pred->get_linear_id()));
   return ls->get_size();
}

size_t node::count_total_all(void) const {
   size_t total(0);
   for(size_t i(0); i < theProgram->num_predicates(); ++i)
      total += count_total(theProgram->get_predicate(i));
   return total;
}

void node::assert_end(void) const {}

void node::dump(ostream &cout) const {
   cout << get_id() << endl;

   for (size_t i(0); i < theProgram->num_predicates(); ++i) {
      predicate *pred(theProgram->get_sorted_predicate(i));

      vector<string> vec;
      if (pred->is_persistent_pred())
         vec = pers_store.dump(pred);
      else {
         if (linear.stored_as_hash_table(pred)) {
            const hash_table *table(
                linear.get_hash_table(pred->get_linear_id()));
            for (hash_table::iterator it(table->begin()); !it.end(); ++it) {
               const intrusive_list<vm::tuple> *ls(db::hash_table::underlying_list(*it));
               if (!ls->empty()) {
                  for (intrusive_list<vm::tuple>::const_iterator
                           it(ls->begin()),
                       end(ls->end());
                       it != end; ++it)
                     vec.push_back((*it)->to_str(pred));
               }
            }
         } else {
            const intrusive_list<vm::tuple> *ls(
                linear.get_linked_list(pred->get_linear_id()));
            if (ls && !ls->empty()) {
               for (intrusive_list<vm::tuple>::const_iterator it(ls->begin()),
                    end(ls->end());
                    it != end; ++it)
                  vec.push_back(to_string((*it)->to_str(pred)));
            }
         }
      }

      if (!vec.empty()) {
         sort(vec.begin(), vec.end());

         for (auto &elem : vec) cout << elem << endl;
      }
   }
}

void node::print(ostream &cout) const {
   cout << "--> node " << get_translated_id() << "/(id " << get_id() << ") <--"
        << endl;

   for (size_t i(0); i < theProgram->num_predicates(); ++i) {
      predicate *pred(theProgram->get_sorted_predicate(i));

      vector<string> vec;
      if (pred->is_persistent_pred())
         vec = pers_store.print(pred);
      else {
         if (linear.stored_as_hash_table(pred)) {
            const hash_table *table(
                linear.get_hash_table(pred->get_linear_id()));
            for (hash_table::iterator it(table->begin()); !it.end(); ++it) {
               const intrusive_list<vm::tuple> *ls(db::hash_table::underlying_list(*it));
               if (!ls->empty()) {
                  for (intrusive_list<vm::tuple>::const_iterator
                           it(ls->begin()),
                       end(ls->end());
                       it != end; ++it)
                     vec.push_back((*it)->to_str(pred));
               }
            }
         } else {
            const intrusive_list<vm::tuple> *ls(
                linear.get_linked_list(pred->get_linear_id()));
            if (ls && !ls->empty()) {
               for (intrusive_list<vm::tuple>::iterator it(ls->begin()),
                    end(ls->end());
                    it != end; ++it) {
                  auto s((*it)->to_str(pred));
                  vec.push_back(s);
               }
            }
         }
      }

      if (vec.empty()) continue;

      cout << " ";
      pred->print_simple(cout);
      cout << endl;

      sort(vec.begin(), vec.end());
      write_strings(vec, cout, 1);
   }
}

void
node::just_moved()
{
   if(theProgram->has_special_fact(vm::special_facts::JUST_MOVED)) {
      vm::predicate *pred(theProgram->get_special_fact(vm::special_facts::JUST_MOVED));
      vm::tuple *tpl(vm::tuple::create(pred, &alloc));
      add_linear_fact(tpl, pred);
   }
}

void
node::just_moved_buffer()
{
   if(theProgram->has_special_fact(vm::special_facts::JUST_MOVED)) {
      vm::predicate *pred(theProgram->get_special_fact(vm::special_facts::JUST_MOVED));
      vm::tuple *tpl(vm::tuple::create(pred, &alloc));
      inner_add_work_others(tpl, pred);
   }
}

ostream &operator<<(ostream &cout, const node &node) {
   node.print(cout);
   return cout;
}
}
