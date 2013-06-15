
#include <cmath>
#include <boost/algorithm/string.hpp>

#include "runtime/list.hpp"
#include "external/lists.hpp"
#include "runtime/string.hpp"
#include "utils/utils.hpp"

using namespace std;
using namespace runtime;
using namespace boost::algorithm;
using namespace utils;

namespace vm
{
namespace external
{
   
argument
intlistlength(EXTERNAL_ARG(ls))
{
	DECLARE_INT_LIST(ls);
   int_val total(0);
   int_list *p((int_list *)ls);

   while(!int_list::is_null(p)) {
      total++;
      p = p->get_tail();
   }

	RETURN_INT(total);
}

static bool inline
has_value(const int_list *_l, int_val v)
{
   int_list *l((int_list*)_l);

   while(!int_list::is_null(l)) {
      if(l->get_head() == v) {
         return true;
      }

      l = l->get_tail();
   }

   return false;
}

argument
intlistdiff(EXTERNAL_ARG(ls1), EXTERNAL_ARG(ls2))
{
   DECLARE_INT_LIST(ls1);
   DECLARE_INT_LIST(ls2);
   
   int_list *p((int_list*)ls1);

   stack_int_list s;

   while(!int_list::is_null(p)) {

      if(!has_value(ls2, p->get_head()))
         s.push(p->get_head());

      p = p->get_tail();
   }

   int_list *ptr(from_stack_to_list<stack_int_list, int_list>(s));
      
   RETURN_INT_LIST(ptr);
}

argument
intlistnth(EXTERNAL_ARG(ls), EXTERNAL_ARG(v))
{
   DECLARE_INT_LIST(ls);
   DECLARE_INT(v);

   int_list *p((int_list*)ls);
   int_val count(0);

   while(!int_list::is_null(p)) {

      if(count == v) {
         int_val r(p->get_head());

         RETURN_INT(r);
      }

      count++;
      p = p->get_tail();
   }

   assert(false);
}

argument
nodelistremove(EXTERNAL_ARG(ls), EXTERNAL_ARG(n))
{
	DECLARE_NODE_LIST(ls);
	DECLARE_NODE(n);

	node_list *p((node_list*)ls);

   stack_node_list s;

   while(!node_list::is_null(p)) {

		if(p->get_head() != n)
			s.push(p->get_head());

      p = p->get_tail();
   }

   node_list *ptr(from_stack_to_list<stack_node_list, node_list>(s));
      
   RETURN_NODE_LIST(ptr);
}

argument
intlistsub(EXTERNAL_ARG(p), EXTERNAL_ARG(a), EXTERNAL_ARG(b))
{
   DECLARE_INT_LIST(p);
   DECLARE_INT(a);
   DECLARE_INT(b);
   int_list *ls((int_list *)p);

   int_val ctn(0);

   while(!int_list::is_null(ls) && ctn < a) {
      ls = ls->get_tail();
      ++ctn;
   }

   stack_int_list s;

   while(!int_list::is_null(ls) && ctn < b) {
      s.push(ls->get_head());

      ls = ls->get_tail();
      ++ctn;
   }

   int_list *ptr(from_stack_to_list<stack_int_list, int_list>(s));

   RETURN_INT_LIST(ptr);
}

argument
intlistappend(EXTERNAL_ARG(ls1), EXTERNAL_ARG(ls2))
{
   DECLARE_INT_LIST(ls1);
   DECLARE_INT_LIST(ls2);

   int_list *p1((int_list*)ls1);
   int_list *p2((int_list*)ls2);

   stack_int_list s;

   while(!int_list::is_null(p1)) {
      s.push(p1->get_head());
      p1 = p1->get_tail();
   }

   while(!int_list::is_null(p2)) {
      s.push(p2->get_head());
      p2 = p2->get_tail();
   }

   int_list *ptr(from_stack_to_list<stack_int_list, int_list>(s));

   RETURN_INT_LIST(ptr);
}

static bool
cmp_ints(const int_val i, const int_val j)
{
   return i > j;
}

argument
intlistsort(EXTERNAL_ARG(ls))
{
   DECLARE_INT_LIST(ls);

   int_list *p((int_list*)ls);
   vector_int_list vec;

   while(!int_list::is_null(p)) {
      vec.push_back(p->get_head());
      p = p->get_tail();
   }

   sort(vec.begin(), vec.end(), cmp_ints);

   int_list *ptr(from_vector_to_reverse_list<vector_int_list, int_list>(vec));

   RETURN_INT_LIST(ptr);
}

argument
intlistremoveduplicates(EXTERNAL_ARG(ls))
{
   DECLARE_INT_LIST(ls);

   int_list *p((int_list*)ls);
   vector_int_list vec;

   while(!int_list::is_null(p)) {
      vec.push_back(p->get_head());
      p = p->get_tail();
   }

   vector<bool, mem::allocator<bool> > marked(vec.size(), true);
   stack_int_list s;

   for(size_t i(0); i < vec.size(); ++i) {
      if(marked[i] == false) {
         continue;
      }
      // this will be kept
      s.push(vec[i]);
      for(size_t j(i+1); j < vec.size(); ++j) {
         if(vec[j] == vec[i])
            marked[j] = false;
      }
   }

   int_list *ptr(from_stack_to_list<stack_int_list, int_list>(s));

   RETURN_INT_LIST(ptr);
}

argument
str2intlist(EXTERNAL_ARG(str))
{
   DECLARE_STRING(str);

   stack_int_list st;
   const string s(str->get_content());

   vector<string> vec;
   split(vec, s, is_any_of(", "), token_compress_on);

   for(vector<string>::const_iterator it(vec.begin()), end(vec.end());
      it != end;
      ++it)
   {
      int_val i;
      from_string(i, *it, std::dec);
      st.push(i);
   }

   int_list *ptr(from_stack_to_list<stack_int_list, int_list>(st));

   RETURN_INT_LIST(ptr);
}

argument
nodelistlength(EXTERNAL_ARG(ls))
{
   DECLARE_NODE_LIST(ls);

   int_val total(0);
   node_list *p((node_list *)ls);

   while(!node_list::is_null(p)) {
      total++;
      p = p->get_tail();
   }

	RETURN_INT(total);
}

argument
nodelistcount(EXTERNAL_ARG(ls), EXTERNAL_ARG(el))
{
   DECLARE_NODE_LIST(ls);
   DECLARE_NODE(el);
   node_list *p((node_list *)ls);
   int_val total(0);

   while(!node_list::is_null(p)){
      if(p->get_head() == el) {
         ++total;
      }
      p = p->get_tail();
   }

   RETURN_INT(total);
}

argument
nodelistappend(EXTERNAL_ARG(ls1), EXTERNAL_ARG(ls2))
{
   DECLARE_NODE_LIST(ls1);
   DECLARE_NODE_LIST(ls2);

   node_list *p1((node_list*)ls1);
   node_list *p2((node_list*)ls2);

   stack_node_list s;

   while(!node_list::is_null(p1)) {
      s.push(p1->get_head());
      p1 = p1->get_tail();
   }

   while(!node_list::is_null(p2)) {
      s.push(p2->get_head());
      p2 = p2->get_tail();
   }

   node_list *ptr(from_stack_to_list<stack_node_list, node_list>(s));

   RETURN_NODE_LIST(ptr);
}

argument
nodelistreverse(EXTERNAL_ARG(ls))
{
   DECLARE_NODE_LIST(ls);
   node_list *p((node_list *)ls);
   stack_node_list s;

   while(!node_list::is_null(p)) {
      s.push(p->get_head());
      p = p->get_tail();
   }

   node_list *ptr(from_stack_to_reverse_list<stack_node_list, node_list>(s));

   RETURN_NODE_LIST(ptr);
}

argument
nodelistlast(EXTERNAL_ARG(ls))
{
   DECLARE_NODE_LIST(ls);
   node_list *p((node_list *)ls);
   node_val last = 0;

   while(!node_list::is_null(p)) {
      last = p->get_head();
      p = p->get_tail();
   }

   RETURN_NODE(last);
}

}
}
