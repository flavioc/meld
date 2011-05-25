
#ifndef SCHED_TYPES_HPP
#define SCHED_TYPES_HPP

namespace process
{
   
enum scheduler_type {
   SCHED_UNKNOWN,
   SCHED_THREADS_STATIC_GLOBAL,
   SCHED_MPI_UNI_STATIC,
   SCHED_THREADS_STATIC_LOCAL,
   SCHED_THREADS_SINGLE_LOCAL,
   SCHED_THREADS_DYNAMIC_LOCAL,
   SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL
};

inline const bool is_mpi_sched(const scheduler_type type)
{
   return type == SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL ||
      type == SCHED_MPI_UNI_STATIC;

}

}

#endif
