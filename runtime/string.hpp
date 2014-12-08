
#ifndef RUNTIME_OBJS_HPP
#error "Please include runtime/objs.hpp instead"
#endif

struct rstring
{
public:
	
	typedef rstring* rstring_ptr;
   typedef rstring_ptr ptr;
	
private:
	
	std::atomic<vm::ref_count> refs;
	std::string content;
	
public:
	
	inline void inc_refs(void)
	{
		refs++;
	}
	
	inline void dec_refs(void)
	{
		assert(refs > 0);
		if(--refs == 0)
         destroy();
	}
	
	inline void destroy(void)
	{
      remove(this);
	}
	
   inline bool has_refs(void) const
   {
      return refs > 0;
   }
	
	inline std::string get_content(void) const
	{
		return content;
	}

   inline rstring_ptr copy(void) const
   {
      return make_string(content);
   }
	
	// XXX implement MPI stuff

	static inline rstring_ptr make_default_string(const std::string& str)
	{
      rstring_ptr p = mem::allocator<rstring>().allocate(1);
      mem::allocator<rstring>().construct(p);
      p->content = str;
      p->refs = 1;
		return p;
	}
	
	static inline rstring_ptr make_string(const std::string& str)
	{
      rstring_ptr p = mem::allocator<rstring>().allocate(1);
      mem::allocator<rstring>().construct(p);
      p->content = str;
		return p;
	}

   static inline void remove(rstring_ptr p)
   {
      mem::allocator<rstring>().destroy(p);
      mem::allocator<rstring>().deallocate(p, 1);
   }

	explicit rstring():
      refs(0)
	{
	}
};

