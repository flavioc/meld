
#include <string>

#include "utils/utils.hpp"
#include "db/trie.hpp"

using namespace vm;
using namespace std;
using namespace runtime;

namespace db
{

size_t
trie_node::count_refs(void) const
{
   if(is_leaf()) {
      simple_tuple *tpl(get_tuple_leaf());
      
      return tpl->get_count();
   } else {
      size_t ret(0);
      
      trie_node *cur(get_child());
      
      while (cur) {
         ret += cur->count_refs();
         
         cur = cur->next;
      }
      
      return ret;
   }
}

size_t
trie_node::delete_by_first_int(const int_val val)
{
   assert(!is_leaf());
   
   if(get_child() == NULL)
      return 0;
      
   trie_node *cur(get_child());
   
   while (cur) {
      if(cur->data.int_field == val) {
         // found it !
      
         const size_t ret(cur->count_refs());
         
         if(cur->prev != NULL)
            cur->prev->next = cur->next;
         if(cur->next != NULL)
            cur->next->prev = cur->prev;
         
         if(cur->prev == NULL)
            this->child = cur->next;
            
         delete cur;
         
         return ret;
      }
      
      cur = cur->get_next();
   }
   
   return 0;
}

trie_node*
trie_node::find_first_leaf(void) const
{
   if(is_leaf())
      return (trie_node*)this;
      
   trie_node *cur(get_child());
   
   if(cur == NULL)
      return NULL;
   
   while(!cur->is_leaf())
      cur = cur->get_child();
      
   return cur;
}

trie_node*
trie_node::find_next_leaf(void) const
{
   assert(is_leaf());
   
   trie_node *cur((trie_node*)this);
   
   while (cur != NULL) {
      trie_node *next(cur->get_next());
   
      if(next) {
         trie_node *leaf(next->find_first_leaf());
         assert(leaf->is_leaf());
         return leaf;
      }
   
      cur = cur->get_parent();
   }
   
   return NULL;
}

trie_node*
trie_node::match(const tuple_field& field, const field_type& typ, val_stack& vals, type_stack& typs) const
{
   assert(!is_leaf());
   
   trie_node *next(get_child());
   
   if(next == NULL) {
      return NULL;
   }
   
   while(next != NULL) {
      tuple_field& f(next->data);
      
      switch(typ) {
         case FIELD_INT:
            if(f.int_field == field.int_field) {
               return next;
            }
            break;
         case FIELD_FLOAT:
            if(f.float_field == field.float_field)
               return next;
            break;
         case FIELD_NODE:
            if(f.node_field == field.node_field)
               return next;
            break;
            
#define MATCH_LIST(LIST_TYPE, FIELD_LIST_TYPE, FIELD_ITEM_TYPE, ITEM_FIELD) { \
            LIST_TYPE *ls((LIST_TYPE*)field.ptr_field);  \
            if(LIST_TYPE::is_null(ls)) { \
               if(f.int_field == 0) return next; \
            } else { \
                  if(f.int_field == 1) { \
                     tuple_field head, tail; \
                     head.ITEM_FIELD = ls->get_head(); \
                     tail.ptr_field = (ptr_val)ls->get_tail(); \
                     vals.push(tail); \
                     typs.push(FIELD_LIST_TYPE); \
                     vals.push(head); \
                     typs.push(FIELD_ITEM_TYPE); \
                     return next; \
                  } \
               } \
            } \
      
         case FIELD_LIST_INT: MATCH_LIST(int_list, FIELD_LIST_INT, FIELD_INT, int_field); break;
         case FIELD_LIST_FLOAT: MATCH_LIST(float_list, FIELD_LIST_FLOAT, FIELD_FLOAT, float_field); break;
         case FIELD_LIST_NODE: MATCH_LIST(node_list, FIELD_LIST_NODE, FIELD_NODE, node_field); break;
         default: assert(false);
      }
      
      next = next->get_next();
   }

   return NULL;
}
   
trie_node*
trie_node::insert(const tuple_field& field, const field_type& type, val_stack& vals, type_stack& typs)
{
   tuple_field f;
   trie_node *new_child;
   
   switch(type) {
#define INSERT_LIST(LIST_TYPE, FIELD_LIST_TYPE, FIELD_ITEM_TYPE, ITEM_FIELD) { \
      LIST_TYPE *ls((LIST_TYPE*)field.ptr_field); \
      if(LIST_TYPE::is_null(ls)) { \
         f.int_field = 0; \
      } else { \
         f.int_field = 1; \
         tuple_field head, tail; \
         head.ITEM_FIELD = ls->get_head(); \
         tail.ptr_field = (ptr_val)ls->get_tail(); \
         vals.push(tail); \
         typs.push(FIELD_LIST_TYPE); \
         vals.push(head); \
         typs.push(FIELD_ITEM_TYPE); \
      } \
   }
         case FIELD_LIST_INT: INSERT_LIST(int_list, FIELD_LIST_INT, FIELD_INT, int_field); break;
         case FIELD_LIST_FLOAT: INSERT_LIST(float_list, FIELD_LIST_FLOAT, FIELD_FLOAT, float_field); break;
         case FIELD_LIST_NODE: INSERT_LIST(node_list, FIELD_LIST_NODE, FIELD_NODE, node_field); break;
         default:
            f = field;
            break;
   }
   
   new_child = new trie_node(f);
   
   assert(!is_leaf());
   
   new_child->parent = this;
   new_child->next = child;
   new_child->prev = NULL;
   
   if(child)
      child->prev = new_child;
   
   child = new_child;
   assert(new_child->get_parent() == this);
   assert(child->get_parent() == this);
   
   return new_child;
}
   
void
trie_node::delete_path(void)
{
   trie_node *parent(get_parent());
   
   if(parent == NULL) { // reached root
      if(is_leaf())
         simple_tuple::wipeout(get_tuple_leaf());
      else {
         if(child)
            delete child;
      }
      child = NULL;
      return;
   }
   
   if(prev != NULL)
      prev->next = next;
   if(next != NULL)
      next->prev = prev;
      
   if(prev == NULL && next == NULL)
      parent->delete_path();
   else {
      if(prev == NULL && next != NULL)
         parent->child = next;
      // delete everything below
      delete this;
   }
}
   
trie_node::~trie_node(void)
{
   if(is_leaf()) {
      simple_tuple *tpl(get_tuple_leaf());
      simple_tuple::wipeout(tpl);
      return;
   }
   
   trie_node *next(get_child());
      
   while(next != NULL) {
      trie_node *tmp(next->get_next());
         
      delete next;
         
      next = tmp;
   }
}

trie_node*
trie::check_insert(vm::tuple *tpl, const ref_count many, bool& found)
{
   // cout << "Starting insertion of " << *tpl << endl;
   
   if(tpl->num_fields() == 0) {
      // 0-arity tuple
      if(!root->is_leaf()) {
         found = false;
         root->set_tuple_leaf(new simple_tuple(tpl, many));
      } else {
         found = true;
         simple_tuple *orig(root->get_tuple_leaf());
         
         orig->add_count(many);
      }
      
      return root;
   }
 
   trie_node::val_stack vals;
   trie_node::type_stack typs;
  
   for(int i(tpl->num_fields()-1); i >= 0; --i) {
      vals.push(tpl->get_field(i));
      typs.push(tpl->get_field_type(i));
   }
   
   trie_node *parent(root);
   
   while (!parent->is_leaf()) {
      assert(!vals.empty() && !typs.empty());
      
      tuple_field field(vals.top());
      field_type typ(typs.top());
      trie_node *cur;
      
      vals.pop();
      typs.pop();
      
      cur = parent->match(field, typ, vals, typs);
      
      if(cur == NULL) {
         // insert
         assert(many > 0);
         assert(!parent->is_leaf());
         
         parent = parent->insert(field, typ, vals, typs);
         
         assert(parent != root);
         
         while(!vals.empty()) {
            field = vals.top();
            typ = typs.top();
            vals.pop();
            typs.pop();
            
            assert(!parent->is_leaf());
            
            parent = parent->insert(field, typ, vals, typs);
         }
         
         assert(vals.empty());
         assert(typs.empty());
         
         // parent is now set as a leaf
         parent->set_tuple_leaf(new simple_tuple(tpl, many));
         
         found = false;
         
         assert(!root->is_leaf());
         
         return parent;
      }
      
      parent = cur;
   }
   
   assert(vals.empty());
   assert(typs.empty());
   assert(!root->is_leaf());
   
   found = true;
   
   assert(parent->is_leaf());
   simple_tuple *orig(parent->get_tuple_leaf());
   
   orig->add_count(many);
   
   return parent;
}

bool
trie::insert_tuple(vm::tuple *tpl, const ref_count many)
{
   /*cout << "To insert " << *tpl << endl;
   
   dump(cout);*/
   
   bool found;
   check_insert(tpl, many, found);
   
   /*cout << "After:" << endl;
   dump(cout);
   cout << endl;*/
   
   total += many;
   
   const bool is_new(!found);
   
   return is_new;
}

void
trie::commit_delete(trie_node *node)
{
   /*simple_tuple *tpl(node->get_tuple_leaf());
   cout << "To delete: " << *tpl << endl;
   dump(cout);*/
   node->delete_path();
   /*cout << "After: " << endl;
   dump(cout);
   printf("\n");*/
}

trie::delete_info
trie::delete_tuple(vm::tuple *tpl, const ref_count many)
{
   assert(many > 0);
   
   bool found;
   trie_node *node(check_insert(tpl, -many, found));
   
   assert(found);
   
   total -= many;
   
   simple_tuple *target(node->get_tuple_leaf());
   
   if(target->reached_zero())
      return delete_info(this, true, node);
   else
      return delete_info(false);
}

tuple_vector*
trie::match_predicate(void) const
{
   tuple_vector *ret(new tuple_vector());
   
   for(const_iterator it(begin());
      it != end();
      it++)
   {
      ret->push_back((*it)->get_tuple());
   }
   
   return ret;
}

void
trie::print(ostream& cout) const
{
   assert(!empty());
   
   cout << " " << *pred << ":" << endl;
   
   for(const_iterator it(begin());
      it != end();
      it++)
   {
      simple_tuple *stuple(*it);
      
      cout << "\t" << *stuple << endl;
   }
}

void
trie::dump(ostream& cout) const
{
   std::list<string> ls;
   
   for(const_iterator it(begin());
      it != end();
      it++)
   {
      simple_tuple *stuple(*it);
      ls.push_back(utils::to_string(*stuple));
   }
   
   ls.sort();
   
   for(std::list<string>::const_iterator it(ls.begin());
      it != ls.end();
      ++it)
   {
      cout << *it << endl;
   }
}

void
trie::delete_by_first_int_arg(const int_val val)
{
   total -= root->delete_by_first_int(val);
}

void
trie::delete_all(void)
{
   assert(false);
   wipeout();
}

void
trie::wipeout(void)
{
   delete root;
   total = 0;
}

trie::trie(const predicate *_pred):
   pred(_pred),
   root(new trie_node()),
   total(0)
{
}

trie::~trie(void)
{
   wipeout();
}
   
}