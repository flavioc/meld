
#include "db/agg_configuration.hpp"

using namespace std;
using namespace vm;
using namespace runtime;

namespace db
{

void
agg_configuration::add_to_set(vm::tuple *tpl, const ref_count many)
{
   assert(many != 0);
   //assert(many > 0 || (many < 0 && !vals.empty()));

   changed = true; // this is important

   const size_t start_size(vals.size());
   
   if(many > 0) {
      
      if(!vals.insert_tuple(tpl, many)) {
         // repeated tuple
         delete tpl;
      }

      assert(vals.size() == start_size + many);
   } else {
      // to delete
      trie::delete_info deleter(vals.delete_tuple(tpl, -many)); // note the minus sign
      if(deleter.to_delete()) {
         deleter();
      }
      
      delete tpl;
   }
}

bool
agg_configuration::test(vm::tuple *tpl, const field_num agg_field) const
{
   if(vals.empty())
      return false;

   const_iterator it(vals.begin());
   
   assert(it != vals.end());
   
   simple_tuple *sother(*it);
   vm::tuple *other(sother->get_tuple());

   for(field_num i(0); i < agg_field; ++i)
      if(!tpl->field_equal(*other, i))
         return false;

   return true;
}

bool
agg_configuration::matches_first_int_arg(const int_val val) const
{
   if(vals.empty())
      return false;
   
   assert(!vals.empty());

   const_iterator it(vals.begin());
   
   assert(it != vals.end());
   
   simple_tuple *sother(*it);
   vm::tuple *other(sother->get_tuple());
   
   return other->get_int(0) == val;
}

vm::tuple*
agg_configuration::generate_max_int(const field_num field) const
{
   assert(!vals.empty());

   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   vm::tuple *max_tpl((*it)->get_tuple());
   int_val max_val(max_tpl->get_int(field));

   ++it;
   for(; it != end; ++it)
   {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());

      if(max_val < other->get_int(field)) {
         max_val = other->get_int(field);
         max_tpl = other;
      }
   }

   return max_tpl->copy();
}

vm::tuple*
agg_configuration::generate_min_int(const field_num field) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   vm::tuple *min_tpl((*it)->get_tuple());
   int_val min_val(min_tpl->get_int(field));

   ++it;
   for(; it != end; ++it) {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());

      if(min_val > other->get_int(field)) {
         min_val = other->get_int(field);
         min_tpl = other;
      }
   }

   return min_tpl->copy();
}

vm::tuple*
agg_configuration::generate_sum_int(const field_num field) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   int_val sum_val = 0;
   vm::tuple *ret((*it)->get_tuple()->copy());

   for(; it != end; ++it) {
      simple_tuple *s(*it);

      for(ref_count i(0); i < s->get_count(); ++i)
         sum_val += s->get_tuple()->get_int(field);
   }

   ret->set_int(field, sum_val);
   return ret;
}

vm::tuple*
agg_configuration::generate_sum_float(const field_num field) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   float_val sum_val = 0;
   vm::tuple *ret((*it)->get_tuple()->copy());

   for(; it != vals.end(); ++it) {
      simple_tuple *s(*it);

      for(ref_count i(0); i < s->get_count(); ++i)
         sum_val += (*it)->get_tuple()->get_float(field);
   }

   ret->set_float(field, sum_val);

   return ret;
}

vm::tuple*
agg_configuration::generate_first(void) const
{
   assert(!vals.empty());
   
   const_iterator fst(vals.begin());
   
   assert(fst != vals.end());
   
   simple_tuple *s(*fst);

   return s->get_tuple()->copy();
}

vm::tuple*
agg_configuration::generate_max_float(const field_num field) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   vm::tuple *max_tpl((*it)->get_tuple());
   float_val max_val(max_tpl->get_float(field));

   ++it;
   for(; it != end; ++it) {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());

      if(max_val < other->get_float(field)) {
         max_val = other->get_float(field);
         max_tpl = other;
      }
   }

   return max_tpl->copy();
}

vm::tuple*
agg_configuration::generate_min_float(const field_num field) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   vm::tuple *min_tpl((*it)->get_tuple());
   float_val min_val(min_tpl->get_float(field));

   ++it;
   for(; it != end; ++it) {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());

      if(min_val > other->get_float(field)) {
         min_val = other->get_float(field);
         min_tpl = other;
      }
   }

   return min_tpl->copy();
}

vm::tuple*
agg_configuration::generate_sum_list_float(const field_num field) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   vm::tuple *first_tpl((*it)->get_tuple());
   float_list *first(first_tpl->get_float_list(field));
   const size_t len(float_list::length(first));
   const size_t num_lists(vals.size());
   float_list *lists[num_lists];
   vm::tuple *ret(first_tpl->copy_except(field));
   
   for(size_t i(0); it != end; ++it) {
      simple_tuple *stpl(*it);
      size_t count(size_t (stpl->get_count()));
      for(size_t j(0); j < count; ++j) {
         lists[i++] = stpl->get_tuple()->get_float_list(field);
         assert(float_list::length(lists[i-1]) == len);
      }
      assert(i <= num_lists);
   }
   
   stack_float_list vals;
   
   for(size_t i(0); i < len; ++i) {
      float_val sum(0.0);
      
      for(size_t j(0); j < num_lists; ++j) {
         float_list *ls(lists[j]);
         assert(ls != NULL);
         sum += ls->get_head();
         lists[j] = ls->get_tail();
      }
      
      vals.push(sum);
   }
   
   for(size_t j(0); j < num_lists; ++j)
      assert(float_list::is_null(lists[j]));
   
   float_list *ptr(from_stack_to_list<stack_float_list, float_list>(vals));
   
   ret->set_float_list(field, ptr);
   
   return ret;
}

vm::tuple*
agg_configuration::do_generate(const aggregate_type typ, const field_num field)
{
   if(vals.empty())
      return NULL;

   switch(typ) {
      case AGG_FIRST:
         return generate_first();
      case AGG_MAX_INT:
         return generate_max_int(field);
      case AGG_MIN_INT:
         return generate_min_int(field);
      case AGG_SUM_INT:
         return generate_sum_int(field);
      case AGG_SUM_FLOAT:
         return generate_sum_float(field);
      case AGG_MAX_FLOAT:
         return generate_max_float(field);
      case AGG_MIN_FLOAT:
         return generate_min_float(field);
      case AGG_SUM_LIST_FLOAT:
         return generate_sum_list_float(field);
   }
   
   assert(false);
   return NULL;
}

void
agg_configuration::generate(const aggregate_type typ, const field_num field,
   simple_tuple_list& cont)
{
   vm::tuple* generated(do_generate(typ, field));
   
   /*
   cout << "Generated from ";
   if(generated)
      cout << *generated;
   cout << endl;
   vals.dump(cout);
   cout << endl;
   */

   changed = false;

   if(corresponds == NULL) {
      // new
      if(generated != NULL)
         cont.push_back(simple_tuple::create_new(generated));
   } else if (generated == NULL) {
      if(corresponds != NULL)
         cont.push_back(simple_tuple::remove_new(corresponds));
   } else {
      if(!(*corresponds == *generated)) {
         cont.push_back(simple_tuple::remove_new(corresponds));
         cont.push_back(simple_tuple::create_new(generated));
      }
   }

   corresponds = generated;
}

agg_configuration::~agg_configuration(void)
{
}

void
agg_configuration::print(ostream& cout) const
{
   for(const_iterator it(vals.begin());
      it != vals.end();
      ++it)
   {
      cout << "\t\t" << **it << endl;
   }
}

ostream&
operator<<(ostream& cout, const agg_configuration& conf)
{
   conf.print(cout);
   return cout;
}
  
}
