
#ifndef VM_SPECIAL_FACTS_HPP
#define VM_SPECIAL_FACTS_HPP

#include <cstdint>

#include "vm/defs.hpp"
#include "vm/predicate.hpp"

namespace vm
{

struct special_facts
{
   using flag_type = std::uint32_t;
   static const flag_type JUST_MOVED{0x1};
   static const flag_type THREAD_LIST{0x2};
   static const flag_type OTHER_THREAD{0x4};
   static const flag_type LEADER_THREAD{0x8};
   flag_type special_facts_flag{0};
#define MARK_SPECIAL_FACT(FLAG) special_facts_flag |= (FLAG)
#define UNMARK_SPECIAL_FACT(FLAG) special_facts_flag &= ~(FLAG)

   inline void mark(const flag_type flag) { MARK_SPECIAL_FACT(flag); }
   inline void unmark(const flag_type flag) { UNMARK_SPECIAL_FACT(flag); }
   inline bool has(const flag_type flag) const { return special_facts_flag & flag; }

#undef MARK_SPECIAL_FACT
#undef UNMARK_SPECIAL_FACT
};

struct special_facts_id
{
   special_facts flags;
   static const size_t MAX_SPECIAL_PREDICATE_IDS{16};
   vm::predicate *ids[MAX_SPECIAL_PREDICATE_IDS];

   inline void mark(const special_facts::flag_type f, const vm::predicate *p) {
      assert(f < MAX_SPECIAL_PREDICATE_IDS);
      ids[f] = (vm::predicate*)p;
      flags.mark(f);
   }

   inline bool has(const special_facts::flag_type f) { return flags.has(f); }

   inline vm::predicate* get(const special_facts::flag_type f) const { return ids[f]; }
};

}

#endif
