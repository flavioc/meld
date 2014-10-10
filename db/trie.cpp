
#include <string>

#include "utils/utils.hpp"
#include "db/trie.hpp"
#include "db/node.hpp"
#include "db/agg_configuration.hpp"

using namespace vm;
using namespace std;
using namespace runtime;

namespace db
{

static const size_t STACK_EXTRA_SIZE(3);
static const size_t TRIE_HASH_LIST_THRESHOLD(8);
static const size_t TRIE_HASH_BASE_BUCKETS(64);

size_t
trie_hash::count_refs(void) const
{
   size_t ret(0);
   
   for(size_t i(0); i < num_buckets; ++i) {
      trie_node *next(buckets[i]);
      
      while (next) {
         ret += next->count_refs();
         next = next->next;
      }
   }
   
   return ret;
}

size_t
trie_node::count_refs(void) const
{
   if(is_leaf()) {
      return get_leaf()->get_count();
   } else {
      size_t ret(0);
      
      trie_node *cur(get_child());
      
      if(is_hashed()) {
         trie_hash *hash((trie_hash*)cur);
         ret += hash->count_refs();
      } else {
         while (cur) {
            ret += cur->count_refs();
         
            cur = cur->next;
         }
      }
      
      return ret;
   }
}

trie_node*
trie_node::get_by_int(const int_val val) const
{
   assert(!is_leaf());
   assert(get_child() != NULL);
   
   trie_node *node(NULL);
   
   if(is_hashed()) {
      trie_hash *hash(get_hash());
      node = hash->get_int(val);
   } else
      node = get_child();
   
   while (node) {
      if(FIELD_INT(node->data) == val)
         // found it !
         return node;
      
      node = node->get_next();
   }
   
   return NULL;
}

trie_node*
trie_node::get_by_float(const float_val val) const
{
   assert(!is_leaf());
   assert(get_child() != NULL);
   
   trie_node *node(NULL);
   
   if(is_hashed()) {
      trie_hash *hash(get_hash());
      node = hash->get_float(val);
   } else
      node = get_child();
   
   while (node) {
      if(FIELD_FLOAT(node->data) == val)
         // found it !
         return node;
      
      node = node->get_next();
   }
   
   return NULL;
}

trie_node*
trie_node::get_by_node(const node_val val) const
{
   assert(!is_leaf());
   assert(get_child() != NULL);
   
   trie_node *node(NULL);
   
   if(is_hashed()) {
      trie_hash *hash(get_hash());
      node = hash->get_node(val);
   } else
      node = get_child();
   
   while (node) {
      if(FIELD_NODE(node->data) == val)
         // found it !
         return node;
      
      node = node->get_next();
   }
   
   return NULL;
}

trie_node*
trie_node::match(const tuple_field& field, type *typ,
      match_stack& mstk, size_t& count) const
{
   assert(!is_leaf());
   
   count = 0;
   
   trie_node *next(get_child());
   
   if(next == NULL)
      return NULL;
      
   if(is_hashed()) {
      trie_hash *hash((trie_hash*)next);
      switch(typ->get_type()) {
         case FIELD_INT: next = hash->get_int(FIELD_INT(field)); break;
         case FIELD_FLOAT: next = hash->get_float(FIELD_FLOAT(field)); break;
         case FIELD_NODE: next = hash->get_node(FIELD_NODE(field)); break;
			case FIELD_STRING: next = hash->get_uint(FIELD_PTR(field)); break;
         case FIELD_LIST: {
            runtime::cons *ls(FIELD_CONS(field));
            next = hash->get_uint(runtime::cons::is_null(ls) ? 0 : 1);
            break;
         }
         default: assert(false);
      }
    }
   
   while(next != NULL) {
      tuple_field& f(next->data);
      
      ++count;
      
      switch(typ->get_type()) {
         case FIELD_INT:
            if(FIELD_INT(f) == FIELD_INT(field)) {
               return next;
            }
            break;
         case FIELD_FLOAT:
            if(FIELD_FLOAT(f) == FIELD_FLOAT(field))
               return next;
            break;
         case FIELD_NODE:
            if(FIELD_NODE(f) == FIELD_NODE(field))
               return next;
            break;
			case FIELD_STRING:
				if(FIELD_STRING(f) == FIELD_STRING(field))
					return next;
				break;
         case FIELD_STRUCT: {
            runtime::struct1 *s(FIELD_STRUCT(field));
            struct_type *st(s->get_type());

            if(FIELD_INT(f) == (int_val)st->get_size()) {
               for(size_t i(st->get_size()); i > 0; --i) {
						match_field f = {false, st->get_type(i-1), s->get_data(i-1)};
						mstk.push(f);
               }
               return next;
            }
         }
         break;
         case FIELD_LIST: {
               runtime::cons *ls(FIELD_CONS(field));
               if(runtime::cons::is_null(ls)) {
                  if(FIELD_PTR(f) == 0)
                     return next;
               } else {
                  if(FIELD_PTR(f) == 1) {
                     tuple_field head, tail;
                     list_type *lt((list_type*)typ);
                     head = ls->get_head();
                     SET_FIELD_CONS(tail, ls->get_tail());
							match_field f_tail = {false, typ, tail};
							match_field f_head = {false, lt->get_subtype(), head};
							mstk.push(f_tail);
							mstk.push(f_head);
                     return next;
                  }
               }
            }
            break;
            
         default: assert(false);
      }
      
      next = next->get_next();
   }

   return NULL;
}
   
trie_node*
trie_node::insert(const tuple_field& field, type *t, match_stack& mstk)
{
   tuple_field f;
   trie_node *new_child;

   // avoid warnings.
   f.ptr_field = 0;
   
   switch(t->get_type()) {
         case FIELD_LIST: {
            runtime::cons *ls(FIELD_CONS(field));
            if(runtime::cons::is_null(ls))
               SET_FIELD_PTR(f, 0);
            else {
               SET_FIELD_PTR(f, 1);

               tuple_field head, tail;
               list_type *lt((list_type*)t);

               head = ls->get_head();
               SET_FIELD_PTR(tail, ls->get_tail());
					const match_field f_tail = {false, lt, tail};
					const match_field f_head = {false, lt->get_subtype(), head};
					mstk.push(f_tail);
					mstk.push(f_head);
            }
            break;
         }

         case FIELD_STRUCT: {
            runtime::struct1 *s(FIELD_STRUCT(field));
            struct_type *st(s->get_type());

            SET_FIELD_INT(f, st->get_size());

            for(size_t i(st->get_size()); i > 0; --i) {
					const match_field f = {false, st->get_type(i-1), s->get_data(i-1)};
					mstk.push(f);
            }
            break;
         }

         case FIELD_BOOL:
         case FIELD_INT:
         case FIELD_FLOAT:
         case FIELD_NODE:
            f = field;
            break;
         default: assert(false); break;
   }
   
   new_child = new trie_node(f);
   
   assert(!is_leaf());
   
   new_child->parent = this;

   if(is_hashed()) {
		assert(child);
		
		trie_hash *hash((trie_hash*)child);
      
      hash->total++;
      
      switch(t->get_type()) {
         case FIELD_LIST: {
               runtime::cons *c(FIELD_CONS(f));

               if(runtime::cons::is_null(c))
                  hash->insert_uint(0, new_child);
               else
                  hash->insert_uint(1, new_child);
            }
            break;
			case FIELD_STRING:
				hash->insert_uint(FIELD_PTR(f), new_child);
				break;
         case FIELD_INT: hash->insert_int(FIELD_INT(f), new_child); break;
         case FIELD_FLOAT: hash->insert_float(FIELD_FLOAT(f), new_child); break;
         case FIELD_NODE: hash->insert_node(FIELD_NODE(f), new_child); break;
         default: assert(false);
      }
   } else {
      new_child->next = child;
      new_child->prev = NULL;
   
      if(child)
         child->prev = new_child;
   
      child = new_child;
   }
   
   assert(new_child->get_parent() == this);
   
   return new_child;
}

// put all children into hash table
void
trie_node::convert_hash(type *type)
{
   assert(!is_hashed());
   
   trie_node *next(get_child());
   trie_hash *hash(new trie_hash(type, this));
   size_t total(0);
   
   while (next != NULL) {
      trie_node *tmp(next->next);
      
      switch(type->get_type()) {
         case FIELD_LIST: {
               runtime::cons *c(FIELD_CONS(next->data));
               if(runtime::cons::is_null(c))
                  hash->insert_uint(0, next);
               else 
                  hash->insert_uint(1, next);
           }
           break;

			case FIELD_STRING:
				hash->insert_uint(FIELD_PTR(next->data), next);
				break;
         case FIELD_INT: hash->insert_int(FIELD_INT(next->data), next); break;
         case FIELD_FLOAT: hash->insert_float(FIELD_FLOAT(next->data), next); break;
         case FIELD_NODE: hash->insert_node(FIELD_NODE(next->data), next); break;
         default: assert(false);
      }
      
      ++total;
      next = tmp;
   }
   
   hash->total = total;
   child = (trie_node*)hash;
	assert(!is_hashed());
	hashed = true;
   
   assert(is_hashed());
}

trie_node::~trie_node(void)
{
}

#define HASH_INT(VAL) (std::hash<int_val>()(VAL))
#define HASH_UINT(VAL) (std::hash<uint_val>()(VAL))
#define HASH_FLOAT(VAL) (std::hash<float_val>()(VAL)/10000)
#ifdef USE_REAL_NODES
#define HASH_NODE(VAL) (std::hash<node::node_id>()(((db::node*)VAL)->get_id()))
#else
#define HASH_NODE(VAL) (std::hash<node_val>()(VAL))
#endif

trie_node*
trie_hash::get_int(const int_val& val) const
{
   return buckets[hash_item(HASH_INT(val))];
}

trie_node*
trie_hash::get_float(const float_val& val) const
{
   return buckets[hash_item(HASH_FLOAT(val))];
}

trie_node*
trie_hash::get_node(const node_val& val) const
{
   return buckets[hash_item(HASH_NODE(val))];
}

trie_node*
trie_hash::get_uint(const uint_val& val) const
{
   return buckets[hash_item(HASH_UINT(val))];
}

void
trie_hash::insert_int(const int_val& val, trie_node *node)
{
   const size_t bucket(hash_item(HASH_INT(val)));
   
   assert(num_buckets > 0);
   assert(bucket < num_buckets);
   
   trie_node *old(buckets[bucket]);
   
   node->prev = NULL;
   node->next = old;
   if(old)
      old->prev = node;
   node->bucket = buckets + bucket;
   
   buckets[bucket] = node;
}

void
trie_hash::insert_uint(const uint_val& val, trie_node *node)
{
	const size_t bucket(hash_item(HASH_UINT(val)));
   
   assert(num_buckets > 0);
   assert(bucket < num_buckets);
   
   trie_node *old(buckets[bucket]);
   
   node->prev = NULL;
   node->next = old;
   if(old)
      old->prev = node;
   node->bucket = buckets + bucket;
   
   buckets[bucket] = node;
}

void
trie_hash::insert_float(const float_val& val, trie_node *node)
{
   const size_t bucket(hash_item(HASH_FLOAT(val)));
   
   assert(bucket < num_buckets);
   
   trie_node *old(buckets[bucket]);
   
   node->prev = NULL;
   node->next = old;
   if(old)
      old->prev = node;
   node->bucket = buckets + bucket;
   
   buckets[bucket] = node;
}

void
trie_hash::insert_node(const node_val& val, trie_node *node)
{
   const size_t bucket(hash_item(HASH_NODE(val)));
   
   assert(bucket < num_buckets);
   
   trie_node *old(buckets[bucket]);
   
   node->prev = NULL;
   node->next = old;
   if(old)
      old->prev = node;
   node->bucket = buckets + bucket;
   
   buckets[bucket] = node;
}

void
trie_hash::expand(void)
{
   const size_t old_num_buckets(num_buckets);
   trie_node **old_buckets(buckets);
   
   num_buckets *= 2;
   buckets = mem::allocator<trie_node*>().allocate(num_buckets);
   memset(buckets, 0, sizeof(trie_node*)*num_buckets);
   
   for(size_t i(0); i < old_num_buckets; ++i) {
      trie_node *next(old_buckets[i]);
      
      while (next) {
         trie_node *tmp(next->next);
         
         switch(type->get_type()) {
				case FIELD_STRING:
					insert_uint(FIELD_PTR(next->data), next);
					break;
            case FIELD_LIST: {
                  runtime::cons *c(FIELD_CONS(next->data));
                  insert_uint(runtime::cons::is_null(c) ? 0 : 1, next);
               }
               break;
            case FIELD_INT: insert_int(FIELD_INT(next->data), next); break;
            case FIELD_FLOAT: insert_float(FIELD_FLOAT(next->data), next); break;
            case FIELD_NODE: insert_node(FIELD_NODE(next->data), next); break;
            default: assert(false); break;
         }
         
         next = tmp;
      }
   }
   
   mem::allocator<trie_node*>().deallocate(old_buckets, old_num_buckets);
}

trie_hash::trie_hash(vm::type *_type, trie_node *):
   type(_type), total(0)
{
   buckets = mem::allocator<trie_node*>().allocate(TRIE_HASH_BASE_BUCKETS);
   num_buckets = TRIE_HASH_BASE_BUCKETS;
   memset(buckets, 0, sizeof(trie_node*)*num_buckets);
}

trie_hash::~trie_hash(void)
{
   mem::allocator<trie_node*>().deallocate(buckets, num_buckets);
}

// deletes the node and also any upper nodes if they lead to this node alone
void
trie::delete_path(trie_node *node)
{  
   trie_node *parent(node->get_parent());
   
   if(node == root) // reached root
   {
      assert(node->child == NULL);
      node->hashed = false;
      return;
   }
   
   assert(node->child == NULL);
   
   if(node->prev != NULL)
      node->prev->next = node->next;
      
   if(node->next != NULL)
      node->next->prev = node->prev;
      
   if(node->bucket != NULL) {
      trie_hash *hash(parent->get_hash());
      hash->total--;
   }
   
   if(node->prev == NULL) {
      if(node->bucket != NULL) {
         *(node->bucket) = node->next;
         
         trie_hash *hash(parent->get_hash());
         
         if(hash->total == 0) {
            delete hash;
				node->hashed = false;
				node->bucket = NULL;
            assert(parent != (trie_node*)hash);
            parent->child = NULL;
            delete_path(parent);
         }
      } else {
         assert(parent->child == node);
         
         parent->child = node->next;
         
         if(parent->child == NULL) {
            assert(node->next == NULL);
            delete_path(parent);
         }
      }
   }
   
   delete node;
}

// deletes everything below the 'node'
size_t
trie::delete_branch(trie_node *node, predicate *pred)
{
   size_t count;
   
   if(node->is_leaf()) {
      trie_leaf *leaf(node->get_leaf());
      
      if(leaf->prev)
         leaf->prev->next = leaf->next;
      if(leaf->next)
         leaf->next->prev = leaf->prev;
      
      if(leaf == first_leaf)
         first_leaf = leaf->next;
      if(leaf == last_leaf)
         last_leaf = leaf->prev;
      
      count = leaf->get_count();
      
      leaf->destroy(pred);
      delete leaf;
      node->child = NULL;
      
      return count;
   }
   
   trie_node *next(node->get_child());
   
   count = 0;
   
   if(node->is_hashed()) {
      trie_hash *hash(node->get_hash());
      
      for(size_t i(0); i < hash->num_buckets; ++i) {
         if(hash->buckets[i]) {
            trie_node *next(hash->buckets[i]);

            while(next != NULL) {
               trie_node *tmp(next->next);

               count += delete_branch(next, pred);
               delete next;

               next = tmp;
            }
         }
      }
   } else {
      while(next != NULL) {
         trie_node *tmp(next->get_next());
         
         count += delete_branch(next, pred);
         delete next;
         
         next = tmp;
      }
   }
   
   node->child = NULL;
   
   return count;
}

void
trie::sanity_check(void) const
{
   //assert(root->count_refs() == (size_t)number_of_references);
}

// inserts the data inside the trie
trie_node*
trie::check_insert(void *data, predicate *pred, const derivation_count many, const depth_t depth, match_stack& mstk, bool& found)
{
   if(mstk.empty()) {
      // 0-arity tuple
      if(!root->is_leaf()) {
         // branch not found
         found = false;
         if(many > 0) {
            trie_leaf *leaf(create_leaf(data, pred, many, depth));
            root->set_leaf(leaf);
            leaf->node = root;
            leaf->prev = leaf->next = NULL;
            first_leaf = last_leaf = leaf;
         }
      } else {
         found = true;
         trie_leaf *leaf(root->get_leaf());
         
         if(many > 0)
            leaf->add_new(depth, many);
         else
            leaf->sub(depth, many);
      }

      return root;
   }
   
   trie_node *parent(root);
   
   while (!parent->is_leaf()) {
      assert(!mstk.empty());
      
		match_field f(mstk.top());
		tuple_field field = f.field;
		type *typ = f.ty;
      trie_node *cur;
      size_t count;
      
		mstk.pop();
      
      cur = parent->match(field, typ, mstk, count);
      
      if(cur == NULL) {
         if(many < 0) {
            return NULL; // tuple not found in the trie
         }
         // else do insertion
         assert(!parent->is_leaf());
         
         ++count;
         
         if(count > TRIE_HASH_LIST_THRESHOLD && !parent->is_hashed()) {
            parent->convert_hash(typ);
            assert(parent->is_hashed());
         } else if (parent->is_hashed()) {
            trie_hash *hsh(parent->get_hash());
            if(count > hsh->total/2 && hsh->total > 5)
               cerr << "Hash table " << hsh << " becoming too imbalanced\n";
            if(count > TRIE_HASH_LIST_THRESHOLD/2)
               hsh->expand();
         }
         
         parent = parent->insert(field, typ, mstk);

         assert(parent != root);
         
         while(!mstk.empty()) {
				f = mstk.top();
            field = f.field;
            typ = f.ty;
				mstk.pop();
            
            assert(!parent->is_leaf());
            parent = parent->insert(field, typ, mstk);
         }
         
         assert(mstk.empty());
         
         // parent is now set as a leaf
         trie_leaf *leaf(create_leaf(data, pred, many, depth));
         leaf->node = parent;
         parent->set_leaf(leaf);
         leaf->next = NULL;
         
         if(first_leaf == NULL) {
            first_leaf = last_leaf = leaf;
            leaf->prev = NULL;
         } else {
            leaf->prev = last_leaf;
            last_leaf->next = leaf;
            last_leaf = leaf;
         }
         
         assert(last_leaf == leaf);
         
         found = false;
         
         assert(!root->is_leaf());
         assert(first_leaf != NULL);
         assert(root->child != NULL);
         
         return parent;
      }
      
      parent = cur;
   }
   
   assert(mstk.empty());
   assert(!root->is_leaf());
   
   found = true;
   
   assert(parent->is_leaf());
   trie_leaf *orig(parent->get_leaf());

   if(many > 0)
      orig->add_new(depth, many);
   else
      orig->sub(depth, many);

   assert(first_leaf != NULL && last_leaf != NULL);
   
   basic_invariants();
   
   return parent;
}

void
trie::commit_delete(trie_node *node, predicate *pred, const ref_count many)
{
   assert(node->is_leaf());

   number_of_references -= many;
   assert(number_of_references >= 0);
   inner_delete_by_leaf(node->get_leaf(), pred, 0, 0);
#if 0
   if(!(number_of_references == 0 || (number_of_references > 0 && first_leaf != NULL))) {
      cout << " =========== > " << number_of_references << endl;
      cout << root->child << endl;
   }
#endif
   basic_invariants();
}

void
trie::delete_by_leaf(trie_leaf *leaf, predicate *pred, const depth_t depth)
{
   sanity_check();
   --number_of_references;
   assert(number_of_references >= 0);
   inner_delete_by_leaf(leaf, pred, 1, depth);
   sanity_check();
}

// we assume that number_of_references was decrement previous to this
void
trie::inner_delete_by_leaf(trie_leaf *leaf, predicate *pred, const derivation_count count, const depth_t depth)
{
   if(count != 0) {
      assert(count > 0);
      leaf->sub(depth, -count);
   }
   
   if(!leaf->to_delete()) {
      return;
   }
 
   assert(leaf->to_delete());
   
   trie_node *node(leaf->node);
   
   if(leaf->next)
      leaf->next->prev = leaf->prev;
   if(leaf->prev)
      leaf->prev->next = leaf->next;
      
   const bool equal_first(leaf == first_leaf);
   if(equal_first) {
      assert(leaf->prev == NULL);
      first_leaf = first_leaf->next;
   }
   
   const bool equal_last(leaf == last_leaf);
   if(equal_last) {
      assert(leaf->next == NULL);
      last_leaf = last_leaf->prev;
   }
      
   if(equal_first && equal_last) {
      assert(last_leaf == NULL);
      assert(first_leaf == NULL);
   }
   
   //cout << this << " Total " << total << " root " << root << " node " << node << endl;
   
   leaf->destroy(pred);
   delete leaf;
   node->child = NULL;
   delete_path(node);
   
   assert(root->next == NULL);
   assert(root->prev == NULL);
}

void
trie::delete_by_index(predicate *pred, const match& m)
{
   basic_invariants();
   
   const size_t stack_size(m.size() + STACK_EXTRA_SIZE);
   match_stack mstk(stack_size);
   
   trie_node *node(root);
   
   // initialize stack
   m.get_match_stack(mstk);
   
   while(!mstk.empty()) {
      match_field f = mstk.top();
      tuple_field mfield = f.field;
      
		mstk.pop();
      
      if(!f.exact)
         break;
      
      switch(f.ty->get_type()) {
         case FIELD_INT:
            node = node->get_by_int(FIELD_INT(mfield));
            break;
         case FIELD_FLOAT:
            node = node->get_by_float(FIELD_FLOAT(mfield));
            break;
         case FIELD_NODE:
            node = node->get_by_node(FIELD_NODE(mfield));
            break;
         default: assert(false);
      }
      if(node == NULL)
         return; // not found
   }
   
   assert(node != NULL);

   // update number of tuples in this trie
   number_of_references -= delete_branch(node, pred);
   assert(number_of_references >= 0);
   delete_path(node);
   
   basic_invariants();
}

void
trie::wipeout(predicate *pred)
{
   delete_branch(root, pred);
   delete root;
   number_of_references = 0;
}

trie::trie(void):
   root(new trie_node()),
   number_of_references(0),
   first_leaf(NULL),
   last_leaf(NULL)
{
   basic_invariants();
}

trie_node*
tuple_trie::check_insert(vm::tuple *tpl, vm::predicate *pred, const derivation_count many, const depth_t depth, bool& found)
{
   //cout << "Starting insertion of " << *tpl << endl;
 
	match_stack mstk(pred->num_fields() + STACK_EXTRA_SIZE);
  
   if(pred->num_fields() > 0) {
      for(int i(pred->num_fields()-1); i >= 0; --i) {
			const match_field f = {false, pred->get_field_type(i), tpl->get_field(i)};
			mstk.push(f);
      }
   }
   
   trie_node *ret(trie::check_insert((void*)tpl, pred, many, depth, mstk, found));

   return ret;
}

bool
tuple_trie::insert_tuple(vm::tuple *tpl, vm::predicate *pred, const derivation_count many, const depth_t depth)
{
   sanity_check();

   bool found;
   number_of_references += many;
   check_insert(tpl, pred, many, depth, found);
   
   if(found) {
      assert(root->child != NULL);
      assert(number_of_references > 0);
   }
   
   sanity_check();
   basic_invariants();
   
   const bool is_new(!found);
   
   return is_new;
}

trie::delete_info
tuple_trie::delete_tuple(vm::tuple *tpl, vm::predicate *pred, const derivation_count many, const depth_t depth)
{
   assert(many > 0);
   basic_invariants();
   
   bool found;
   trie_node *node(check_insert(tpl, pred, -many, depth, found));

   if(node == NULL) {
      // already deleted
      return delete_info(NULL, this, false, NULL, 0);
   }
   
   assert(found);
   
   trie_leaf *leaf(node->get_leaf());
   
   if(leaf->to_delete()) {
      // for this branch, we will decrease number_of_references later
      // in commit_delete
      return delete_info((tuple_trie_leaf*)leaf, this, true, node, many);
   } else {
      number_of_references -= many;
      return delete_info((tuple_trie_leaf*)leaf, this, false, node, many);
   }
}

vector<string>
tuple_trie::get_print_strings(predicate *pred) const
{
   vector<string> vec;
   for(const_iterator it(begin());
      it != end();
      it++)
   {
      tuple_trie_leaf *leaf(*it);
      if(leaf->to_delete())
         continue;
      string str = leaf->get_underlying_tuple()->to_str(pred);
      vec.push_back(str);
      if(leaf->get_count() > 1) {
         for(size_t i(1); i < leaf->get_count(); ++i) {
            vec.push_back(str);
         }
      }
   }
   return vec;
}

void
tuple_trie::print(ostream& cout, predicate *pred) const
{ 
   assert(!empty());
   utils::write_strings(get_print_strings(pred), cout, 1);
}

tuple_trie::tuple_search_iterator
tuple_trie::match_predicate(void) const
{
	return tuple_search_iterator((tuple_trie_leaf*)first_leaf);
}

void
tuple_trie::do_visit(trie_node *n, const int tab, stack<type*>& s) const
{
   for(int i(0); i < tab; ++i)
      cout << " ";

   assert(!s.empty());
   type *t = s.top();

   cout << "pop " << s.size() << endl;
   s.pop();

   int pushes = 0;
   switch(t->get_type()) {
      case FIELD_FLOAT: cout << "float " << FIELD_FLOAT(n->data) << endl; break;
      case FIELD_INT: cout << "int " << FIELD_INT(n->data) << endl; break;
      case FIELD_NODE: cout << "node " << FIELD_NODE(n->data) << endl; break;
      case FIELD_LIST: {
         runtime::cons *l(FIELD_CONS(n->data));
         if(runtime::cons::is_null(l)) {
            cout << "]" << endl;
         } else {
            cout << "[" << endl;
            list_type *lt((list_type*)t);
            type *st(lt->get_subtype());
            s.push(t);
            s.push(st);
            pushes++;
            pushes++;
         }
      }
      break;
      default: cout << "bad type " << endl; assert(false);
   }

   if(!n->is_leaf()) {
      trie_node *c = n->get_child();
      while(c) {
         do_visit(c, tab + 1, s);
         c = c->next;
      }
   } else {
      cout << "Got a leaf" << endl;
   }

   for(int i(0); i < pushes; ++i)
      s.pop();

   if(t) {
      s.push(t);
   }
}

void
tuple_trie::visit(trie_node *n, predicate *pred) const
{
   stack<type*> types;
   for(int i(pred->num_fields()-1); i >= 0; --i) {
      types.push(pred->get_field_type(i));
   }

   cout << "root" << endl;
   n = n->child;
   while(n) {
      do_visit(n, 1, types);
      n = n->next;
   }
}

#define ADD_ALT(PARENT, NODE) do { \
   assert((NODE) != NULL); \
	frm.parent = (PARENT);	\
   frm.node = NODE;   		\
   frm.mstack = mstk;   	\
   cont_stack.push(frm); } while(false)

void
tuple_trie::tuple_search_iterator::find_next(trie_continuation_frame& frm, const bool force_down)
{
	trie_node *parent = NULL;
	trie_node *node = NULL;
	match_field f;
   tuple_field mfield;
   bool going_down = true;
	match_stack mstk;

	goto restore;

try_again:
	// use continuation stack to restore state at a given node

	if(cont_stack.empty()) {
		next = NULL;
		return;
	}

	frm = cont_stack.top();
	cont_stack.pop();

restore:

	node = frm.node;
	parent = frm.parent;
	mstk = frm.mstack;
	going_down = false || force_down;
	
match_begin:
	assert(!mstk.empty());
   
   f = mstk.top();
   
   assert(node != NULL);
   assert(parent != NULL);
   
   if(f.exact) {
      // must do an exact match
      // there will be no continuation frames at this level
      mfield = f.field;
      switch(f.ty->get_type()) {
         case FIELD_LIST:
            if(parent->is_hashed()) {
               assert(going_down);
               trie_hash *hash((trie_hash*)node);
               if(FIELD_PTR(mfield) == 1)
                  node = hash->get_uint(1);
               else if(FIELD_PTR(mfield) == 0)
                  node = hash->get_uint(0);
               else
                  node = hash->get_uint(1);
            }

            if(FIELD_PTR(mfield) == 1) {
               while(node) {
                  if(FIELD_PTR(node->data) == 1) {
                     list_type *lt((list_type*)f.ty);
                     match_field f_head = {false, lt->get_subtype(), tuple_field()};
                     match_field f_tail = {false, f.ty, tuple_field()};
                     mstk.top() = f_tail; // we just substitute the top frame
                     mstk.push(f_head);
                     goto match_succeeded;
                  }
                  node = node->next;
               }
            } else if(FIELD_PTR(mfield) == 0) {
               while(node) {
                  if(FIELD_PTR(node->data) == 0)
                     goto match_succeeded_and_pop;
                  node = node->next;
               }
            } else {
               while(node) {
                  if(FIELD_PTR(node->data) == 1) {
                     list_match *lm(FIELD_LIST_MATCH(mfield));
                     mstk.top() = lm->tail;
                     mstk.push(lm->head);
                     goto match_succeeded;
                  }
                  node = node->next;
               }
            }
            goto try_again;

         case FIELD_INT:
            if(parent->is_hashed()) {
               assert(going_down);
               trie_hash *hash((trie_hash*)node);
               
               node = hash->get_int(FIELD_INT(mfield));
            }
            
            while(node) {
               if(FIELD_INT(node->data) == FIELD_INT(mfield))
                  goto match_succeeded_and_pop;
               node = node->next;
            }
            goto try_again;
         case FIELD_FLOAT:
            if(parent->is_hashed()) {
               assert(going_down);
               
               trie_hash *hash((trie_hash*)node);
               
               node = hash->get_float(FIELD_FLOAT(mfield));
            }
            
            while(node) {
               if(FIELD_FLOAT(node->data) == FIELD_FLOAT(mfield))
                  goto match_succeeded_and_pop;
               node = node->next;
            }
            goto try_again;
         case FIELD_NODE:
            if(parent->is_hashed()) {
               assert(going_down);
               
               trie_hash *hash((trie_hash*)node);
               
               node = hash->get_node(FIELD_NODE(mfield));
            }
            
            while(node) {
               if(FIELD_NODE(node->data) == FIELD_NODE(mfield))
                  goto match_succeeded_and_pop;
               node = node->next;
            }
            goto try_again;
         default: assert(false);
      }
   } else {
      if(parent->is_hashed()) {
         trie_hash *hash((trie_hash*)node);
         
         if(going_down) {
            // must select a valid node
            trie_node **buckets(hash->buckets);
            trie_node **end_buckets(buckets + hash->num_buckets);
            
            // find first valid bucket
            for(; *buckets == NULL && buckets < end_buckets; ++buckets) {
               if(*buckets)
                  break;
            }
            
            assert(buckets < end_buckets);
            
            node = *buckets;
            assert(node != NULL);
            
            if(node->next)
               ADD_ALT(parent, node->next);
            else {
               // find another valid bucket
               ++buckets;
               for(; *buckets == NULL && buckets < end_buckets; ++buckets)
                  ;
               
               if(buckets != end_buckets)
                  ADD_ALT(parent, *buckets);
            }
         } else {
            // must continue traversing the hash table
            assert(parent->child != node);
            
            trie_hash *hash(parent->get_hash());
            
            assert(hash != NULL);
            
            trie_node **buckets(hash->buckets);
            trie_node **end_buckets(buckets + hash->num_buckets);
            trie_node **current_bucket(node->bucket);
            
            if(node->next)
               ADD_ALT(parent, node->next);
            else {
               // must find new bucket
               ++current_bucket;
               for(; *current_bucket == NULL && current_bucket < end_buckets; ++current_bucket)
                  ;
               if(current_bucket != end_buckets)
                  ADD_ALT(parent, *current_bucket);
            }
         }
      } else {
         // push first alternative
         if(node->next)
            ADD_ALT(parent, node->next);
      }
      
      switch(f.ty->get_type()) {
         case FIELD_LIST:
            if(FIELD_PTR(node->data) == 0) {
               goto match_succeeded_and_pop;
            } else {
               list_type *lt((list_type*)f.ty);
               const match_field f = {false, lt->get_subtype(), tuple_field()};
					mstk.push(f);
               goto match_succeeded;
            }
            break;
         case FIELD_INT:
         case FIELD_FLOAT:
         case FIELD_NODE:
			case FIELD_STRING:
            goto match_succeeded_and_pop;
         default: goto match_succeeded;
      }
   }
   
match_succeeded_and_pop:
   mstk.pop();

match_succeeded:
	if(node->is_leaf()) {
      assert(node != NULL);
      assert(node->is_leaf());

      if(((tuple_trie_leaf*)node->get_leaf())->get_count() == 0)
         goto try_again;

      tuple_trie_leaf *leaf((tuple_trie_leaf*)node->get_leaf());
      next = leaf; // NEW
      return;
	}
   
   parent = node;
   node = node->child;
   going_down = true;
   goto match_begin;
}

tuple_trie::tuple_search_iterator
tuple_trie::match_predicate(const match* m) const
{
   if(!m->any_exact)
      return match_predicate();
   
   if(number_of_references == 0)
      return tuple_search_iterator();
   
   const size_t stack_size(m->size() + STACK_EXTRA_SIZE);
	trie_continuation_frame first_frm = {match_stack(stack_size), root, root->child};
   
   // initialize stacks
   m->get_match_stack(first_frm.mstack);

   // dump(cout);
   // if it was a leaf, m would have no exact match
   assert(!root->is_leaf());

	return tuple_search_iterator(first_frm);
}

agg_trie_leaf::~agg_trie_leaf(void)
{
   assert(conf);
   delete conf;
}

agg_trie_leaf*
agg_trie::find_configuration(vm::tuple *tpl, vm::predicate *pred)
{
   const size_t stack_size(pred->get_aggregate_field() + STACK_EXTRA_SIZE);
   match_stack mstk(stack_size);
  
   for(int i(pred->get_aggregate_field()-1); i >= 0; --i) {
		const match_field f = {false, pred->get_field_type(i), tpl->get_field(i)};
		mstk.push(f);
   }
   
   bool found;
   trie_node *node(trie::check_insert(NULL, pred, 1, 0, mstk, found));
   
   if(!found)
      ++number_of_references;
   
   return (agg_trie_leaf*)node->get_leaf();
}

agg_trie_iterator
agg_trie::erase(agg_trie_iterator& it, predicate *pred)
{
   agg_trie_leaf *leaf(it.current_leaf);
   agg_trie_leaf *next_leaf((agg_trie_leaf*)leaf->next);
   trie_node *node(leaf->node);
   
   leaf->set_zero_refs();
   commit_delete(node, pred, 1);
   
   return agg_trie_iterator(next_leaf);
}
   
}
