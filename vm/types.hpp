#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstring>
#include <string>
#include <stdexcept>
#include <vector>
#include <iostream>

namespace vm {
   
enum field_type {
   FIELD_INT = 0x0,
   FIELD_FLOAT = 0x1,
   FIELD_NODE = 0x2,
   FIELD_LIST = 0x3,
   FIELD_STRUCT = 0x4,
   FIELD_BOOL = 0x5,
	FIELD_STRING = 0x9
};

size_t field_type_size(field_type type);
std::string field_type_string(field_type type);

class type
{
   private:

      field_type ftype;

   public:

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

      explicit type(const field_type _ftype): ftype(_ftype) {}

      virtual ~type(void) { }
};

class list_type: public type
{
   private:

      type *sub_type;

   public:

      inline type *get_subtype(void) const { return sub_type; }

      virtual std::string string(void) const
      {
         return type::string() + " " + sub_type->string();
      }

      virtual bool equal(type *other) const
      {
         if(!type::equal(other))
            return false;

         list_type *other2((list_type*)other);

         return sub_type->equal(other2->sub_type);
      }

      explicit list_type(type *_sub_type):
         type(FIELD_LIST), sub_type(_sub_type)
      {}

      virtual ~list_type(void) { delete sub_type; }
};

class struct_type: public type
{
   private:

      std::vector<type*> types;

   public:

      inline size_t get_size(void) const { return types.size(); }

      inline void set_type(const size_t i, type *t) { types[i] = t; }

      inline type* get_type(const size_t i) const { return types[i]; }

      virtual bool equal(type *other) const
      {
         if(!type::equal(other))
            return false;

         struct_type *sother((struct_type*)other);
         if(get_size() != sother->get_size())
            return false;

         for(size_t i(0); i < get_size(); ++i) {
            if(!types[i]->equal(sother->types[i]))
               return false;
         }

         return true;
      }

      virtual std::string string(void) const
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

      virtual ~struct_type(void)
      {
         for(size_t i(0); i < get_size(); ++i)
            delete types[i];
      }
};

extern type *TYPE_INT, *TYPE_FLOAT, *TYPE_NODE, *TYPE_STRING;
extern list_type *TYPE_LIST_FLOAT, *TYPE_LIST_INT, *TYPE_LIST_NODE;

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
