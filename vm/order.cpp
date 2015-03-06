#include "utils/intrusive_list.hpp"
#include "vm/tuple.hpp"

typedef struct {
   vm::tuple *tpl;
   utils::intrusive_list<vm::tuple> *ls;
   utils::intrusive_list<vm::tuple>::iterator iterator;
} iter_object;
typedef std::vector<iter_object, mem::allocator<iter_object>> vector_iter;

static inline bool sort_tuples(const tuple *t1, const tuple *t2,
                               const vm::field_num field, const vm::predicate *pred) {
   assert(t1 != nullptr && t2 != nullptr);

   switch (pred->get_field_type(field)->get_type()) {
      case FIELD_INT:
         return t1->get_int(field) < t2->get_int(field);
      case FIELD_FLOAT:
         return t1->get_float(field) < t2->get_float(field);
      case FIELD_NODE:
         return t1->get_node(field) < t2->get_node(field);
      default:
         abort();
         break;
   }
   return false;
}

class tuple_sorter {
   private:
   const vm::predicate *pred;
   const vm::field_num field;

   public:
   inline bool operator()(const iter_object &l1, const iter_object &l2) {
      return sort_tuples(l1.tpl, l2.tpl, field, pred);
   }

   explicit tuple_sorter(const vm::field_num _field, const vm::predicate *_pred)
       : pred(_pred), field(_field) {}
};

class tuple_leaf_sorter {
   private:
   const predicate *pred;
   const field_num field;

   public:
   inline bool operator()(const tuple_trie_leaf *l1,
                          const tuple_trie_leaf *l2) {
      return sort_tuples(l1->get_underlying_tuple(), l2->get_underlying_tuple(),
                         field, pred);
   }

   explicit tuple_leaf_sorter(const field_num _field, const predicate *_pred)
       : pred(_pred), field(_field) {}
};

typedef std::vector<tuple_trie_leaf *, mem::allocator<tuple_trie_leaf *>>
    vector_leaves;
