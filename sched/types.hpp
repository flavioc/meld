
#ifndef SCHED_TYPES_HPP
#define SCHED_TYPES_HPP

namespace sched
{
   
enum scheduler_type {
   SCHED_UNKNOWN,
   SCHED_THREADS_STATIC_LOCAL,
   SCHED_THREADS_STATIC_BUFF,
   SCHED_THREADS_SINGLE_LOCAL,
   SCHED_THREADS_DYNAMIC_LOCAL,
   SCHED_THREADS_DIRECT_LOCAL,
   SCHED_THREADS_STATIC_LOCAL_PRIO,
   SCHED_MPI_AND_THREADS_STATIC_LOCAL,
   SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL,
   SCHED_MPI_AND_THREADS_SINGLE_LOCAL,
   SCHED_SERIAL_LOCAL,
	SCHED_SERIAL_UI_LOCAL
};

inline bool is_serial_sched(const scheduler_type type)
{
   return type == SCHED_SERIAL_LOCAL;
}

inline bool is_mpi_sched(const scheduler_type type)
{
   return type == SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL ||
      type == SCHED_MPI_AND_THREADS_STATIC_LOCAL ||
      type == SCHED_MPI_AND_THREADS_SINGLE_LOCAL;
}

inline bool is_work_stealing_sched(const scheduler_type type)
{
   return type == SCHED_THREADS_DYNAMIC_LOCAL ||
      type == SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL ||
      type == SCHED_THREADS_DIRECT_LOCAL;
}

inline bool is_indirect_work_stealing_sched(const scheduler_type type)
{
   return type == SCHED_THREADS_DYNAMIC_LOCAL ||
      type == SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL;
}

}

#endif
