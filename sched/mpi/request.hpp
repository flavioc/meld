
#ifndef SCHED_MPI_REQUEST_HPP
#define SCHED_MPI_REQUEST_HPP

#include "conf.hpp"

#ifdef COMPILE_MPI

#include <utility>
#include <list>
#include <boost/mpi/request.hpp>

namespace process
{

typedef std::pair<boost::mpi::request, utils::byte*> pair_req;
typedef std::list<pair_req> vector_reqs;

}

#endif

#endif

