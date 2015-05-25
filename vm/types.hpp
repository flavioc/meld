#ifndef TYPES_HPP
#define TYPES_HPP

#include <assert.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <vector>
#include <iostream>

#include "vm/defs.hpp"

namespace vm {
   
enum field_type {
   FIELD_INT = 0x0,
   FIELD_FLOAT = 0x1,
   FIELD_NODE = 0x2,
   FIELD_LIST = 0x3,
   FIELD_STRUCT = 0x4,
   FIELD_BOOL = 0x5,
   FIELD_THREAD = 0x6,
   FIELD_ARRAY = 0x7,
   FIELD_SET = 0x8,
	FIELD_STRING = 0x9,
   FIELD_ANY = 0xA
};

inline size_t lookup_type_size(const field_type typ)
{
   switch(typ) {
      case FIELD_LIST:
      case FIELD_STRUCT:
      case FIELD_STRING:
      case FIELD_NODE:
      case FIELD_ARRAY:
      case FIELD_SET:
         return sizeof(vm::ptr_val);
      case FIELD_INT:
         return sizeof(vm::int_val);
      case FIELD_FLOAT:
         return sizeof(vm::float_val);
      default:
         abort();
         return -1;
   }

}

inline bool reference_type(const field_type typ)
{
   switch(typ) {
      case FIELD_LIST:
      case FIELD_STRUCT:
      case FIELD_STRING:
      case FIELD_NODE:
      case FIELD_ARRAY:
      case FIELD_SET:
         return true;
      default:
         return false;
   }
}

size_t field_type_size(field_type type);
std::string field_type_string(field_type type);

class type
{
   private:

      field_type ftype;

   public:

      virtual bool is_reference() const { return false; }

      inline field_type get_type(void) const { return ftype; }

      inline size_t size(void) const
      {
         return field_type_size(ftype);
      }

      virtual std::string string(void) const
      {
         return field_type_string(ftype);
      }

      virtual bool equal(type *other) const
      {
         return ftype == other->ftype;
      }

      explicit type(const field_type _ftype): ftype(_ftype)
      {
      }

      virtual ~type(void) { }
};

class list_type: public type
{
   private:

      type *sub_type;

   public:

      virtual bool is_reference() const override { return true; }

      inline type *get_subtype(void) const { return sub_type; }

      virtual std::string string(void) const override
      {
         return type::string() + " " + sub_type->string();
      }

      virtual bool equal(type *other) const override
      {
         if(!type::equal(other))
            return false;

         list_type *other2((list_type*)other);

         return sub_type->equal(other2->sub_type);
      }

      explicit list_type(type *_sub_type):
         type(FIELD_LIST), sub_type(_sub_type)
      {}

      // sub types must be deleted elsewhere.
      virtual ~list_type(void) { }
};

class struct_type: public type
{
   private:

      std::vector<type*> types;
      bool simple = true;

   public:

      virtual bool is_reference() const override { return true; }

      inline size_t get_size(void) const { return types.size(); }

      inline void set_type(const size_t i, type *t) {
         if(reference_type(t->get_type()))
            simple = false;
         types[i] = t;
      }

      inline type* get_type(const size_t i) const { return types[i]; }

      inline bool simple_composed_type(void) const { return simple; }

      virtual bool equal(type *other) const override
      {
         if(!type::equal(other))
            return false;

         if(struct_type *sother = dynamic_cast<struct_type*>(other)) {
            if(get_size() != sother->get_size())
               return false;

            for(size_t i(0); i < get_size(); ++i) {
               if(!types[i]->equal(sother->types[i]))
                  return false;
            }
         }

         return false;
      }

      virtual std::string string(void) const override
      {
         std::string ret(type::string());

         for(size_t i(0); i < get_size(); ++i) {
            ret += " " + types[i]->string();
         }

         return ret;
      }

      explicit struct_type(const size_t _size):
         type(FIELD_STRUCT)
      {
         types.resize(_size);
      }

      explicit struct_type(std::vector<type*>  _types):
         type(FIELD_STRUCT),
         types(std::move(_types))
      {
         for(type *t : types) {
            if(reference_type(t->get_type())) {
               simple = false;
               break;
            }
         }
      }

      // sub types must be deleted elsewhere.
      virtual ~struct_type(void)
      {
      }
};

class set_type: public type
{
   private:

      type *base;

   public:

      virtual bool is_reference() const override { return true; }

      inline type *get_base() const { return base; }

      virtual std::string string() const override
      {
         return type::string() + " " + base->string();
      }

      virtual bool equal(type *other) const override
      {
         if(!type::equal(other))
            return false;

         if(set_type *sother = dynamic_cast<set_type*>(other))
            return base->equal(sother->base);
         return false;
      }

      explicit set_type(vm::type *_base):
         type(FIELD_SET),
         base(_base)
      {
         assert(base);
      }

      // sub types must be deleted elsewhere.
      virtual ~set_type()
      {}
};

class array_type: public type
{
   private:

      type *base;

   public:

      virtual bool is_reference() const override { return true; }

      inline type *get_base() const { return base; }

      virtual std::string string(void) const override
      {
         return type::string() + " " + base->string();
      }

      virtual bool equal(type *other) const override
      {
         if(!type::equal(other))
            return false;

         if(array_type *sother = dynamic_cast<array_type*>(other))
            return base->equal(sother->base);
         return false;
      }

      explicit array_type(vm::type *_base):
         type(FIELD_ARRAY),
         base(_base)
      {
         assert(base);
      }

      // sub types must be deleted elsewhere.
      virtual ~array_type()
      {}
};

extern type *TYPE_INT, *TYPE_BOOL, *TYPE_FLOAT, *TYPE_NODE, *TYPE_THREAD,
            *TYPE_STRING, *TYPE_ANY, *TYPE_STRUCT, *TYPE_LIST, *TYPE_ARRAY,
            *TYPE_SET, *TYPE_TYPE;
extern list_type *TYPE_LIST_FLOAT, *TYPE_LIST_INT, *TYPE_LIST_NODE, *TYPE_LIST_THREAD;
extern array_type *TYPE_ARRAY_INT;

void init_types(void);

enum aggregate_type {
   AGG_FIRST = 1,
   AGG_MAX_INT = 2,
   AGG_MIN_INT = 3,
   AGG_SUM_INT = 4,
   AGG_MAX_FLOAT = 5,
   AGG_MIN_FLOAT = 6,
   AGG_SUM_FLOAT = 7,
   AGG_SUM_LIST_FLOAT = 11
};

enum aggregate_safeness {
   AGG_LOCALLY_GENERATED = 1,
   AGG_NEIGHBORHOOD = 2,
   AGG_NEIGHBORHOOD_AND_SELF = 3,
   AGG_IMMEDIATE = 4,
   AGG_UNSAFE = 5
};

std::string aggregate_type_string(aggregate_type type);

inline bool aggregate_safeness_uses_neighborhood(const aggregate_safeness safeness)
{
   return safeness == AGG_NEIGHBORHOOD || safeness == AGG_NEIGHBORHOOD_AND_SELF;
}

class type_error : public std::runtime_error {
 public:
    explicit type_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};

}

#endif
