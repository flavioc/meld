
#ifndef RUNTIME_STRING_HPP
#define RUNTIME_STRING_HPP

#include "conf.hpp"

#include <iostream>
#include <string>
#include <stack>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif
#include "utils/types.hpp"
#include "utils/atomic.hpp"
#include "mem/base.hpp"

#include "vm/defs.hpp"


namespace runtime
{

class rstring: public mem::base
{
public:
	
	typedef rstring* rstring_ptr;
   typedef rstring_ptr ptr;
	
	MEM_METHODS(rstring)
	
private:
	
	const std::string content;
	utils::atomic<size_t> refs;
	
	explicit rstring(const std::string& _content, const size_t _refs):
		content(_content), refs(_refs)
	{
	}
	
public:
	
	inline void inc_refs(void)
	{
		refs++;
	}
	
	inline void dec_refs(void)
	{
		assert(refs > 0);
		refs--;
		if(zero_refs())
         destroy();
	}
	
	inline void destroy(void)
	{
		assert(zero_refs());
      delete this;
	}
	
	inline bool zero_refs(void) const
	{
		return refs == 0;
	}
	
	inline std::string get_content(void) const
	{
		return content;
	}
	
	// XXX implement MPI stuff

	static rstring_ptr make_default_string(const std::string& str)
	{
		return new rstring(str, 1);
	}
	
	static rstring_ptr make_string(const std::string& str)
	{
		return new rstring(str, 0);
	}
};

}

#endif
