
#include <cmath>

#include "runtime/list.hpp"
#include "external/math.hpp"

using namespace std;
using namespace runtime;

namespace vm
{
namespace external
{
   
argument
sigmoid(EXTERNAL_ARG(x))
{
   DECLARE_FLOAT(x);
   
   const float_val ret(1.0/(1.0 + exp(-x)));
   
	RETURN_FLOAT(ret);
}

argument
normalize(EXTERNAL_ARG(x))
{
	DECLARE_FLOAT_LIST(x);
   float_list *ptr((float_list*)x);
   
   if(float_list::is_null(ptr)) {
		RETURN_FLOAT_LIST(x);
   }
   
   /* find max value */
   float_val max_value(ptr->get_head());
   ptr = ptr->get_tail();
   while(!float_list::is_null(ptr)) {
      const float_val val(ptr->get_head());

      assert(!isnan(val));
   
      if(val > max_value)
         max_value = val;
         
      ptr = ptr->get_tail();
   }
   
   float_val Z(0.0);
   ptr = (float_list*)x;
   
   while(!float_list::is_null(ptr)) {
      Z += std::exp(ptr->get_head() - max_value);
      ptr = ptr->get_tail();
   }
   
   const float_val logZ(std::log(Z));
   ptr = (float_list*)x;
   stack_float_list vals;
   while(!float_list::is_null(ptr)) {
      vals.push(ptr->get_head() - max_value - logZ);
      ptr = ptr->get_tail();  
   }
   
   float_list *ls(from_stack_to_list<stack_float_list, float_list>(vals));
   
	RETURN_FLOAT_LIST(ls);
}

argument
damp(EXTERNAL_ARG(ls1), EXTERNAL_ARG(ls2), EXTERNAL_ARG(fact))
{
   DECLARE_FLOAT_LIST(ls1);
   DECLARE_FLOAT_LIST(ls2);
   DECLARE_FLOAT(fact);
   float_list *ptr1((float_list*)ls1);
   float_list *ptr2((float_list*)ls2);
   
   float_list *nil(float_list::null_list());
   
   if(float_list::is_null(ptr1) || float_list::is_null(ptr2)) {
		RETURN_FLOAT_LIST(nil);
   }
   
   stack_float_list vals;
   
   while(!float_list::is_null(ptr1) && !float_list::is_null(ptr2)) {
      const float_val h1(ptr1->get_head());
      const float_val h2(ptr2->get_head());
      const float_val c(std::log(fact * std::exp(h2) +
         (1.0 - fact) * std::exp(h1)));
      
      vals.push(c);
      
      ptr1 = ptr1->get_tail();
      ptr2 = ptr2->get_tail();
   }
   
   float_list *ptr(from_stack_to_list<stack_float_list,float_list>(vals));
   
	RETURN_FLOAT_LIST(ptr);
}

argument
divide(EXTERNAL_ARG(ls1), EXTERNAL_ARG(ls2))
{
   DECLARE_FLOAT_LIST(ls1);
   DECLARE_FLOAT_LIST(ls2);
   
   float_list *ptr1((float_list*)ls1);
   float_list *ptr2((float_list*)ls2);
   
   stack_float_list vals;
   
   while(!float_list::is_null(ptr1) && !float_list::is_null(ptr2)) {
      assert(!isnan(ptr1->get_head()));
      assert(!isnan(ptr2->get_head()));
      vals.push(ptr1->get_head() - ptr2->get_head());
      
      ptr1 = ptr1->get_tail();
      ptr2 = ptr2->get_tail();
   }
   
   float_list *ptr(from_stack_to_list<stack_float_list, float_list>(vals));
      
	RETURN_FLOAT_LIST(ptr);
}

argument
convolve(EXTERNAL_ARG(bin_fact), EXTERNAL_ARG(ls))
{
   DECLARE_FLOAT_LIST(bin_fact);
   DECLARE_FLOAT_LIST(ls);
   const size_t length(float_list::length((float_list*)ls));
   
   stack_float_list vals;
   
   for(size_t x(0); x < length; ++x) {
      float_val sum(0.0);
      float_list *ptr((float_list*)ls);
      
      size_t y(0);
      while(!float_list::is_null(ptr)) {
         const float_val other(ptr->get_head());
         const float_val val_bin(float_list::get((float_list*)bin_fact, x + y * length, 0.0));

         assert(!isnan(other));
         assert(!isnan(val_bin));
         sum += std::exp(val_bin + other);
         
         ptr = ptr->get_tail();
         ++y;
      }
      
      if(sum == 0) sum = std::numeric_limits<float_val>::min();
      
      sum = std::log(sum);
      
      vals.push(sum);
   }
   
   float_list *ptr(from_stack_to_list<stack_float_list, float_list>(vals));
      
	RETURN_FLOAT_LIST(ptr);
}

argument
addfloatlists(EXTERNAL_ARG(ls1), EXTERNAL_ARG(ls2))
{
   DECLARE_FLOAT_LIST(ls1);
   DECLARE_FLOAT_LIST(ls2);
   
   float_list *ptr1((float_list*)ls1);
   float_list *ptr2((float_list*)ls2);
   
   stack_float_list vals;
   
   while(!float_list::is_null(ptr1) && !float_list::is_null(ptr2)) {
      assert(!isnan(ptr1->get_head()));
      assert(!isnan(ptr2->get_head()));
      vals.push(ptr1->get_head() + ptr2->get_head());
      
      ptr1 = ptr1->get_tail();
      ptr2 = ptr2->get_tail();
   }
   
   float_list *ptr(from_stack_to_list<stack_float_list, float_list>(vals));
      
   RETURN_FLOAT_LIST(ptr);
}

argument
residual(EXTERNAL_ARG(l1), EXTERNAL_ARG(l2))
{
   DECLARE_FLOAT_LIST(l1);
   DECLARE_FLOAT_LIST(l2);
   float_list *ls1((float_list*)l1);
   float_list *ls2((float_list*)l2);

   double residual(0.0);
   size_t size(0);

   while(!float_list::is_null(ls1) && !float_list::is_null(ls2)) {
      assert(!isnan(ls1->get_head()));
      assert(!isnan(ls2->get_head()));
      residual += std::abs(std::exp(ls1->get_head()) - std::exp(ls2->get_head()));
      ls1 = ls1->get_tail();
      ls2 = ls2->get_tail();
      ++size;
   }

   residual /= (double)size;

   RETURN_FLOAT(residual);
}

argument
intpower(EXTERNAL_ARG(n1), EXTERNAL_ARG(n2))
{
   DECLARE_INT(n1);
   DECLARE_INT(n2);

   int_val result(1);
   for(int i(0); i < n2; ++i)
      result *= n1;

   RETURN_INT(result);
}

}
}
