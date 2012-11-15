
#ifndef PROCESS_WORK_HPP
#define PROCESS_WORK_HPP

#include "utils/types.hpp"
#include "mem/base.hpp"

namespace process
{
   
typedef utils::byte work_modifier;

namespace mods
{   
const work_modifier FORCE_AGGREGATE(0x01);
const work_modifier IS_LOCAL(0x02);
const work_modifier LOCAL_TUPLE(0x04);
const work_modifier NOTHING(0x00);
}

class work;

class node_work /* XXX: make it forbidden to do new/delete */
{
protected:
   
   friend class work;
   
   db::simple_tuple *tuple;
   work_modifier mod;
   
public:
   
   inline vm::tuple* get_underlying_tuple(void) const { return get_tuple()->get_tuple(); }
   inline db::simple_tuple* get_tuple(void) const { return tuple; }
   inline vm::strat_level get_strat_level(void) const { return tuple->get_strat_level(); }
   inline bool force_aggregate(void) const { return mod & mods::FORCE_AGGREGATE; }
   inline bool locally_generated(void) const { return mod & mods::LOCAL_TUPLE; }
   
   explicit node_work(db::simple_tuple *_tuple, const work_modifier _mod):
      tuple(_tuple), mod(_mod)
   {
   }
   
   explicit node_work(db::simple_tuple *_tuple):
      tuple(_tuple), mod(mods::NOTHING)
   {
   }
   
   explicit node_work(void):
      tuple(NULL), mod(mods::NOTHING)
   {
   }
   
   ~node_work(void) {}
};


class work: public node_work
{
private:
   
   db::node *node;
   bool use_rules;
   
public:
   
   inline bool using_rules(void) const { return use_rules; }

   inline db::node* get_node(void) const { return node; }
   
   void copy_from_node(db::node *_node, node_work& w)
   {
      node = _node;
      tuple = w.tuple;
      mod = w.mod;
   }

   void set_work_with_rules(db::node *_node)
   {
      node = _node;
      use_rules = true;
   }
   
   explicit work(db::node *_node, db::simple_tuple *_tuple,
      const work_modifier _mod):
      node_work(_tuple, _mod),
      node(_node)
   {
   }
   
   explicit work(db::node *_node, db::simple_tuple *_tuple):
      node_work(_tuple), node(_node), use_rules(false)
   {
   }
   
   explicit work(void):
      node_work(), node(NULL), use_rules(false)
   {
   }

   ~work(void) {}
};

}

#endif
