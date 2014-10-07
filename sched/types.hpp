
#ifndef SCHED_TYPES_HPP
#define SCHED_TYPES_HPP

namespace sched
{
   
enum scheduler_type {
   SCHED_UNKNOWN,
   SCHED_THREADS
#ifdef USE_UI
	, SCHED_SERIAL_UI
#endif
#ifdef USE_SIM
	, SCHED_SIM
#endif
};

inline bool is_mpi_sched(const scheduler_type)
{
   return false;
}

}

#endif
