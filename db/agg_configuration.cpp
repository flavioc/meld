
#include "db/agg_configuration.hpp"

using namespace std;
using namespace vm;
using namespace runtime;

namespace db
{

void
agg_configuration::add_to_set(vm::tuple *tpl, vm::predicate *pred,
      const derivation_direction dir, const depth_t depth, mem::node_allocator *alloc,
      candidate_gc_nodes& gc_nodes)
{
   changed = true; // this is important

#ifndef NDEBUG
   const size_t start_size(vals.size());
#endif
   
   switch(dir) {
      case POSITIVE_DERIVATION:
         if(!vals.insert_tuple(tpl, pred, depth)) {
            // repeated tuple
            vm::tuple::destroy(tpl, pred, alloc, gc_nodes);
         }

         assert(vals.size() == start_size + 1);
         break;
      case NEGATIVE_DERIVATION:
         // to delete
         trie::delete_info deleter(vals.delete_tuple(tpl, pred, depth)); // note the minus sign
         if(!deleter.is_valid()) {
            changed = false;
         } else if(deleter.to_delete()) {
            deleter.perform_delete(pred, alloc, gc_nodes);
         } else if(pred->is_cycle_pred()) {
            depth_counter *dc(deleter.get_depth_counter());
            assert(dc != nullptr);

            if(dc->get_count(depth) == 0) {
#ifndef NDEBUG
               size_t old_size(vals.size());
#endif
               vm::ref_count deleted(deleter.delete_depths_above(depth));
               (void)deleted;
               if(deleter.to_delete())
                  deleter.perform_delete(pred, alloc, gc_nodes);
               assert(vals.size() == old_size-deleted);
            }
         }
         
         vm::tuple::destroy(tpl, pred, alloc, gc_nodes);
         break;
   }
}

bool
agg_configuration::test(vm::predicate *pred, vm::tuple *tpl, const field_num agg_field) const
{
   if(vals.empty())
      return false;

   const_iterator it(vals.begin());
   
   assert(it != vals.end());
   
   tuple_trie_leaf *sother(*it);
   vm::tuple *other(sother->get_underlying_tuple());

   for(field_num i(0); i < agg_field; ++i)
      if(!tpl->field_equal(pred->get_field_type(i), *other, i))
         return false;

   return true;
}

bool
agg_configuration::matches_first_int_arg(predicate *pred, const int_val val) const
{
   (void)pred;

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
agg_configuration::generate_max_int(predicate *pred, const field_num field, vm::depth_t& depth,
      mem::node_allocator *alloc) const
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

   return max_tpl->copy(pred, alloc);
}

vm::tuple*
agg_configuration::generate_min_int(predicate *pred, const field_num field, vm::depth_t& depth,
      mem::node_allocator *alloc) const
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

   return min_tpl->copy(pred, alloc);
}

vm::tuple*
agg_configuration::generate_sum_int(predicate *pred, const field_num field, vm::depth_t& depth,
      mem::node_allocator *alloc) const
{
   assert(!vals.empty());
   (void)pred;
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   int_val sum_val = 0;
   vm::tuple *ret((*it)->get_underlying_tuple()->copy(pred, alloc));

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
agg_configuration::generate_sum_float(predicate *pred, const field_num field, vm::depth_t& depth,
      mem::node_allocator *alloc) const
{
   assert(!vals.empty());
   (void)pred;
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   float_val sum_val = 0;
   vm::tuple *ret((*it)->get_underlying_tuple()->copy(pred, alloc));

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
agg_configuration::generate_first(predicate *pred, vm::depth_t& depth, mem::node_allocator *alloc) const
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

   return s->get_underlying_tuple()->copy(pred, alloc);
}

vm::tuple*
agg_configuration::generate_max_float(predicate *pred, const field_num field, vm::depth_t& depth,
      mem::node_allocator *alloc) const
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

   return max_tpl->copy(pred, alloc);
}

vm::tuple*
agg_configuration::generate_min_float(predicate *pred, const field_num field, vm::depth_t& depth,
      mem::node_allocator *alloc) const
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

   return min_tpl->copy(pred, alloc);
}

vm::tuple*
agg_configuration::generate_sum_list_float(predicate *pred, const field_num field, vm::depth_t& depth,
      mem::node_allocator *alloc) const
{
   assert(!vals.empty());
   
   const_iterator end(vals.end());
   const_iterator it(vals.begin());
   vm::tuple *first_tpl((*it)->get_underlying_tuple());
   cons *first(first_tpl->get_cons(field));
   const size_t len(cons::length(first));
   const size_t num_lists(vals.size());
   cons *lists[num_lists];
   vm::tuple *ret(first_tpl->copy_except(pred, field, alloc));
   
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
         assert(ls != nullptr);
         sum += FIELD_FLOAT(ls->get_head());
         lists[j] = ls->get_tail();
      }
      
      vals.push(sum);
   }
   
   for(size_t j(0); j < num_lists; ++j)
      assert(cons::is_null(lists[j]));
   
   cons *ptr(from_float_stack_to_list(vals));
   
   ret->set_cons(field, ptr);
   (void)pred;
   
   return ret;
}

vm::tuple*
agg_configuration::do_generate(predicate *pred, const aggregate_type typ, const field_num field,
      vm::depth_t& depth, mem::node_allocator *alloc)
{
   if(vals.empty())
      return nullptr;

   switch(typ) {
      case AGG_FIRST:
         return generate_first(pred, depth, alloc);
      case AGG_MAX_INT:
         return generate_max_int(pred, field, depth, alloc);
      case AGG_MIN_INT:
         return generate_min_int(pred, field, depth, alloc);
      case AGG_SUM_INT:
         return generate_sum_int(pred, field, depth, alloc);
      case AGG_SUM_FLOAT:
         return generate_sum_float(pred, field, depth, alloc);
      case AGG_MAX_FLOAT:
         return generate_max_float(pred, field, depth, alloc);
      case AGG_MIN_FLOAT:
         return generate_min_float(pred, field, depth, alloc);
      case AGG_SUM_LIST_FLOAT:
         return generate_sum_list_float(pred, field, depth, alloc);
   }
   
   assert(false);
   return nullptr;
}

void
agg_configuration::generate(predicate *pred, const aggregate_type typ, const field_num field,
   full_tuple_list& cont, mem::node_allocator *alloc)
{
   vm::depth_t depth(0);
   vm::tuple* generated(do_generate(pred, typ, field, depth, alloc));
   
   changed = false;

   if(corresponds == nullptr) {
      // new
      if(generated != nullptr) {
         cont.push_back(full_tuple::create_new(generated->copy(pred, alloc), pred, depth));
         last_depth = depth;
         corresponds = generated;
      }
   } else if (generated == nullptr) {
      if(corresponds != nullptr) {
         // the user of corresponds will delete it.
         cont.push_back(full_tuple::remove_new(corresponds, pred, last_depth));
         last_depth = 0;
         corresponds = nullptr;
      }
   } else {
      if(!(corresponds->equal(*generated, pred))) {
         cont.push_back(full_tuple::remove_new(corresponds, pred, last_depth));
         cont.push_back(full_tuple::create_new(generated->copy(pred, alloc), pred, depth));
         last_depth = depth;
         corresponds = generated;
      }
   }
}

void
agg_configuration::print(ostream& cout, predicate *pred) const
{
   for(const_iterator it(vals.begin());
      it != vals.end();
      ++it)
   {
      tuple_trie_leaf *leaf(*it);
      cout << "\t\t";
      leaf->get_underlying_tuple()->print(cout, pred);
      cout << "@";
      cout << leaf->get_count();
      if(leaf->has_depth_counter()) {
         cout << " - ";
         for(auto it(leaf->get_depth_begin()), end(leaf->get_depth_end());
               it != end;
               ++it)
         {
            cout << "(" << it->first << "x" << it->second << ")";
         }
      }
      cout << endl;
   }
}

}
