
#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include <string>
#include <vector>

#include "vm/types.hpp"
#include "vm/defs.hpp"

namespace vm {

const size_t PRED_ARGS_MAX = 32;
const size_t NAME_SIZE_MAX = 32;

class predicate {
private:
   static predicate_id current_id;
   
   predicate_id id;
   std::string name;
   
   std::vector<field_type> types;
   std::vector<size_t> fields_size;
   std::vector<size_t> fields_offset;
   
   size_t tuple_size;
   
   typedef struct {
      field_num field;
      aggregate_type type;
   } aggregate_info;
   
   aggregate_info *agg_info;
   
   void build_field_info(void);
   
   explicit predicate(void);
   
public:
   
   inline bool is_aggregate(void) const { return agg_info != NULL; }
   
   inline const field_num get_aggregate_field(void) const { return agg_info->field; }
   inline const aggregate_type get_aggregate_type(void) const { return agg_info->type; }
   
   inline predicate_id get_id(void) const { return id; }
   
   inline size_t num_fields(void) const { return types.size(); }
   
   inline field_type get_field_type(const field_num field) const { return types[field]; }
   inline size_t get_field_size(const field_num field) const { return fields_size[field]; }
   
   inline std::string get_name(void) const { return name; }
   
   inline size_t get_size(void) const { return tuple_size; }
   
   void print(std::ostream&) const;
   
   static predicate* make_predicate_from_buf(unsigned char *buf, code_size_t *code_size);
};

std::ostream& operator<<(std::ostream&, const predicate&);

}

#endif
