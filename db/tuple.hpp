
#ifndef DB_TUPLE_HPP
#define DB_TUPLE_HPP

#include "conf.hpp"

#include <vector>
#include <ostream>
#include <list>

#ifdef COMPILE_MPI
#include <boost/serialization/serialization.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#endif

#include "vm/defs.hpp"
#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "mem/allocator.hpp"
#include "mem/base.hpp"

namespace db
{
   
class simple_tuple: public mem::base<simple_tuple>
{
private:
   vm::tuple *data;

   vm::ref_count count;

#ifdef COMPILE_MPI
   friend class boost::serialization::access;

   void save(boost::mpi::packed_oarchive&, const unsigned int) const;
   void load(boost::mpi::packed_iarchive&, const unsigned int);

   BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

public:

   inline vm::tuple* get_tuple(void) const { return data; }

   void print(std::ostream&) const;

   inline vm::ref_count get_count(void) const { return count; }
   
   inline const bool reached_zero(void) const { return get_count() == 0; }

   inline void inc_count(const vm::ref_count& inc) { count += inc; }

   inline void dec_count(const vm::ref_count& inc) { count -= inc; }
   
   inline void add_count(const vm::ref_count& plus) { count += plus; }

   static simple_tuple* create_new(vm::tuple *tuple) { return new simple_tuple(tuple, 1); }

   static simple_tuple* remove_new(vm::tuple *tuple) { return new simple_tuple(tuple, -1); }
   
   static void wipeout(simple_tuple *stpl) { delete stpl->get_tuple(); delete stpl; }

   explicit simple_tuple(vm::tuple *_tuple, const vm::ref_count _count):
      data(_tuple), count(_count)
   {}

   explicit simple_tuple(void) {} // for serialization purposes

   ~simple_tuple(void);
};

std::ostream& operator<<(std::ostream&, const simple_tuple&);

typedef std::list<simple_tuple*, mem::allocator<simple_tuple*> > simple_tuple_list;
   
class agg_configuration: public mem::base<agg_configuration>
{
private:
   
   simple_tuple_list values;
   bool changed;
   vm::tuple *corresponds;
   
   vm::tuple *generate_max_int(const vm::field_num) const;
   vm::tuple *generate_min_int(const vm::field_num) const;
   vm::tuple *generate_sum_int(const vm::field_num) const;
   vm::tuple *generate_sum_float(const vm::field_num) const;
   vm::tuple *generate_first(void) const;
   vm::tuple *do_generate(const vm::aggregate_type, const vm::field_num);

public:
   
   void print(std::ostream&) const;
   
   void generate(const vm::aggregate_type, const vm::field_num, simple_tuple_list&);
   
   const bool test(vm::tuple *, const vm::field_num) const;
   
   inline const bool has_changed(void) const { return changed; }
   inline const bool is_empty(void) const { return values.empty(); }
   
   void add_to_set(vm::tuple *, const vm::ref_count);
   
   explicit agg_configuration(void):
      changed(false), corresponds(NULL)
   {
   }
   
   ~agg_configuration(void);
};

std::ostream& operator<<(std::ostream&, const agg_configuration&);

class tuple_aggregate: public mem::base<tuple_aggregate>
{
private:
   
   typedef std::list<agg_configuration*, mem::allocator<agg_configuration*> > agg_conf_list;
   
   const vm::predicate *pred;
   agg_conf_list values;
   
public:
   
   void print(std::ostream&) const;
   
   simple_tuple_list generate(void);
   
   void add_to_set(vm::tuple *, const vm::ref_count);
   
   explicit tuple_aggregate(const vm::predicate *_pred): pred(_pred) {}
   
   ~tuple_aggregate(void);
};

std::ostream& operator<<(std::ostream&, const tuple_aggregate&);

}

#ifdef COMPILE_MPI
BOOST_CLASS_TRACKING(db::simple_tuple, boost::serialization::track_never)
#endif

#endif
