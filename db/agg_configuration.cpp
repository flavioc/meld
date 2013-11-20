
#include "db/agg_configuration.hpp"

using namespace std;
using namespace vm;
using namespace runtime;

namespace db
{

void
agg_configuration::add_to_set(vm::tuple *tpl, const derivation_count many, const depth_t depth)
{
   assert(many != 0);
   //assert(many > 0 || (many < 0 && !vals.empty()));

   changed = true; // this is important

#ifndef NDEBUG
   const size_t start_size(vals.size());
#endif
   
   if(many > 0) {
      
      if(!vals.insert_tuple(tpl, many, depth)) {
         // repeated tuple
         delete tpl;
      }

      assert(vals.size() == start_size + many);
   } else {
      // to delete
      trie::delete_info deleter(vals.delete_tuple(tpl, -many, depth)); // note the minus sign
      if(!deleter.is_valid()) {
         changed = false;
      } else if(deleter.to_delete()) {
         deleter();
      } else if(tpl->is_cycle()) {
         depth_counter *dc(deleter.get_depth_counter());
         assert(dc != NULL);

         if(dc->get_count(depth) == 0) {
#ifndef NDEBUG
            size_t old_size(vals.size());
#endif
            vm::ref_count deleted(deleter.delete_depths_above(depth));
            (void)deleted;
            if(deleter.to_delete()) {
               deleter();
            }
            assert(vals.size() == old_size-deleted);
         }
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
   
   tuple_trie_leaf *sother(*it);
   vm::tuple *other(sother->get_underlying_tuple());

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
   
   tuple_trie_leaf *sother(*it);
   vm::tuple *other(sother->get_underlying_tuple());
   
   return other->get_int(0) == val;
}

vm::tuple*
agg_configuration::generate_max_int(const field_num field, vm::depth_t& depth) const
{
   assert(!vals.empty());

   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   tuple_trie_leaf *leaf(*it);
   vm::tuple *max_tpl(leaf->get_underlying_tuple());
   int_val max_val(max_tpl->get_int(field));

   depth = max(depth, leaf->get_max_depth());

   ++it;
   for(; it != end; ++it)
   {
      tuple_trie_leaf *sother(*it);
      vm::tuple *other(sother->get_underlying_tuple());

      depth = max(depth, sother->get_max_depth());
      if(max_val < other->get_int(field)) {
         max_val = other->get_int(field);
         max_tpl = other;
      }
   }

   return max_tpl->copy();
}

vm::tuple*
agg_configuration::generate_min_int(const field_num field, vm::depth_t& depth) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   tuple_trie_leaf *leaf(*it);
   vm::tuple *min_tpl(leaf->get_underlying_tuple());
   int_val min_val(min_tpl->get_int(field));

   depth = max(depth, leaf->get_max_depth());

   ++it;
   for(; it != end; ++it) {
      tuple_trie_leaf *sother(*it);
      vm::tuple *other(sother->get_underlying_tuple());

      depth = max(depth, sother->get_max_depth());
      if(min_val > other->get_int(field)) {
         min_val = other->get_int(field);
         min_tpl = other;
      }
   }

   return min_tpl->copy();
}

vm::tuple*
agg_configuration::generate_sum_int(const field_num field, vm::depth_t& depth) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   int_val sum_val = 0;
   vm::tuple *ret((*it)->get_underlying_tuple()->copy());

   for(; it != end; ++it) {
      tuple_trie_leaf *s(*it);

      depth = max(depth, s->get_max_depth());
      for(ref_count i(0); i < s->get_count(); ++i)
         sum_val += s->get_underlying_tuple()->get_int(field);
   }

   ret->set_int(field, sum_val);
   return ret;
}

vm::tuple*
agg_configuration::generate_sum_float(const field_num field, vm::depth_t& depth) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   float_val sum_val = 0;
   vm::tuple *ret((*it)->get_underlying_tuple()->copy());

   for(; it != vals.end(); ++it) {
      tuple_trie_leaf *s(*it);

      depth = max(depth, s->get_max_depth());
      for(ref_count i(0); i < s->get_count(); ++i)
         sum_val += (*it)->get_underlying_tuple()->get_float(field);
   }

   ret->set_float(field, sum_val);

   return ret;
}

vm::tuple*
agg_configuration::generate_first(vm::depth_t& depth) const
{
   assert(!vals.empty());
   
   const_iterator fst(vals.begin());
   const_iterator end(vals.end());
   
   assert(fst != vals.end());
   
   tuple_trie_leaf *s(*fst);

   for(; fst != end; ++fst) {
      tuple_trie_leaf *sother(*fst);
      depth = max(depth, sother->get_max_depth());
   }

   return s->get_underlying_tuple()->copy();
}

vm::tuple*
agg_configuration::generate_max_float(const field_num field, vm::depth_t& depth) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   tuple_trie_leaf *leaf(*it);
   vm::tuple *max_tpl(leaf->get_underlying_tuple());
   float_val max_val(max_tpl->get_float(field));

   depth = max(depth, leaf->get_max_depth());

   ++it;
   for(; it != end; ++it) {
      tuple_trie_leaf *sother(*it);
      vm::tuple *other(sother->get_underlying_tuple());

      depth = max(sother->get_max_depth(), depth);
      if(max_val < other->get_float(field)) {
         max_val = other->get_float(field);
         max_tpl = other;
      }
   }

   return max_tpl->copy();
}

vm::tuple*
agg_configuration::generate_min_float(const field_num field, vm::depth_t& depth) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   tuple_trie_leaf *leaf(*it);
   vm::tuple *min_tpl(leaf->get_underlying_tuple());
   float_val min_val(min_tpl->get_float(field));

   depth = max(depth, leaf->get_max_depth());

   ++it;
   for(; it != end; ++it) {
      tuple_trie_leaf *sother(*it);
      vm::tuple *other(sother->get_underlying_tuple());

      depth = max(sother->get_max_depth(), depth);
      if(min_val > other->get_float(field)) {
         min_val = other->get_float(field);
         min_tpl = other;
      }
   }

   return min_tpl->copy();
}

vm::tuple*
agg_configuration::generate_sum_list_float(const field_num field, vm::depth_t& depth) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   vm::tuple *first_tpl((*it)->get_underlying_tuple());
   cons *first(first_tpl->get_cons(field));
   const size_t len(cons::length(first));
   const size_t num_lists(vals.size());
   cons *lists[num_lists];
   vm::tuple *ret(first_tpl->copy_except(field));
   
   for(size_t i(0); it != end; ++it) {
      tuple_trie_leaf *stpl(*it);
      size_t count(size_t (stpl->get_count()));
      depth = max(stpl->get_max_depth(), depth);
      for(size_t j(0); j < count; ++j) {
         lists[i++] = stpl->get_underlying_tuple()->get_cons(field);
         assert(cons::length(lists[i-1]) == len);
      }
      assert(i <= num_lists);
   }
   
   stack_float_list vals;
   
   for(size_t i(0); i < len; ++i) {
      float_val sum(0.0);
      
      for(size_t j(0); j < num_lists; ++j) {
         cons *ls(lists[j]);
         assert(ls != NULL);
         sum += FIELD_FLOAT(ls->get_head());
         lists[j] = ls->get_tail();
      }
      
      vals.push(sum);
   }
   
   for(size_t j(0); j < num_lists; ++j)
      assert(cons::is_null(lists[j]));
   
   cons *ptr(from_float_stack_to_list(vals));
   
   ret->set_cons(field, ptr);
   
   return ret;
}

vm::tuple*
agg_configuration::do_generate(const aggregate_type typ, const field_num field, vm::depth_t& depth)
{
   if(vals.empty())
      return NULL;

   switch(typ) {
      case AGG_FIRST:
         return generate_first(depth);
      case AGG_MAX_INT:
         return generate_max_int(field, depth);
      case AGG_MIN_INT:
         return generate_min_int(field, depth);
      case AGG_SUM_INT:
         return generate_sum_int(field, depth);
      case AGG_SUM_FLOAT:
         return generate_sum_float(field, depth);
      case AGG_MAX_FLOAT:
         return generate_max_float(field, depth);
      case AGG_MIN_FLOAT:
         return generate_min_float(field, depth);
      case AGG_SUM_LIST_FLOAT:
         return generate_sum_list_float(field, depth);
   }
   
   assert(false);
   return NULL;
}

void
agg_configuration::generate(const aggregate_type typ, const field_num field,
   simple_tuple_list& cont)
{
   vm::depth_t depth(0);
   vm::tuple* generated(do_generate(typ, field, depth));
   
#if 0
   if(generated) {
      cout << "Generated from ";
         cout << *generated << " " << depth;
      cout << endl;
      vals.dump(cout);
      cout << endl;
   }
#endif

   changed = false;

   if(corresponds == NULL) {
      // new
      if(generated != NULL) {
         cont.push_back(simple_tuple::create_new(generated, depth));
         last_depth = depth;
      }
   } else if (generated == NULL) {
      if(corresponds != NULL) {
         cont.push_back(simple_tuple::remove_new(corresponds, last_depth));
         last_depth = 0;
      }
   } else {
      if(!(*corresponds == *generated)) {
         cont.push_back(simple_tuple::remove_new(corresponds, last_depth));
         cont.push_back(simple_tuple::create_new(generated, depth));
         last_depth = depth;
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
      tuple_trie_leaf *leaf(*it);
      cout << "\t\t";
      cout << *(leaf->get_underlying_tuple()) << "@";
      cout << leaf->get_count();
      if(leaf->has_depth_counter()) {
         cout << " - ";
         for(depth_counter::const_iterator it(leaf->get_depth_begin()), end(leaf->get_depth_end());
               it != end;
               ++it)
         {
            cout << "(" << it->first << "x" << it->second << ")";
         }
      }
      cout << endl;
   }
}

ostream&
operator<<(ostream& cout, const agg_configuration& conf)
{
   conf.print(cout);
   return cout;
}
  
}
