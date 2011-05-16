
#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include <string>
#include <vector>
#include <assert.h>

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "utils/types.hpp"

namespace vm {

const size_t PRED_DESCRIPTOR_BASE_SIZE = 4;
const size_t PRED_ARGS_MAX = 32;
const size_t PRED_NAME_SIZE_MAX = 32;
const size_t PRED_AGG_INFO_MAX = 32;

class predicate {
private:
   static predicate_id current_id;
   
   predicate_id id;
   std::string name;
   strat_level level;
   
   std::vector<field_type> types;
   std::vector<size_t> fields_size;
   std::vector<size_t> fields_offset;
   
   size_t tuple_size;
   
   typedef struct {
      field_num field;
      aggregate_type type;
      std::vector<predicate_id> sizes;
   } aggregate_info;
   
   aggregate_info *agg_info;
   
   void build_field_info(void);
   
   explicit predicate(void);
   
public:
   
   static strat_level MAX_STRAT_LEVEL;
   
   inline bool is_aggregate(void) const { return agg_info != NULL; }
   
   inline bool has_agg_term_info(void) const { assert(is_aggregate()); return !agg_info->sizes.empty(); }
   
   std::vector<const predicate*> get_agg_deps(void) const;
   
   inline const field_num get_aggregate_field(void) const { return agg_info->field; }
   inline const aggregate_type get_aggregate_type(void) const { return agg_info->type; }
   
   inline predicate_id get_id(void) const { return id; }
   
   inline size_t num_fields(void) const { return types.size(); }
   
   inline field_type get_field_type(const field_num field) const { return types[field]; }
   inline size_t get_field_size(const field_num field) const { return fields_size[field]; }
   
   inline std::string get_name(void) const { return name; }
   
   inline size_t get_size(void) const { return tuple_size; }
   
   inline strat_level get_strat_level(void) const { return level; }
   
   void print(std::ostream&) const;
   
   static predicate* make_predicate_from_buf(utils::byte *buf, code_size_t *code_size);
};

std::ostream& operator<<(std::ostream&, const predicate&);

}

#endif
