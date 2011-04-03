
#ifndef PROCESS_EXEC_HPP
#define PROCESS_EXEC_HPP

#include <string>

#include "process/process.hpp"

namespace process
{

void execute_file(const std::string&, const size_t num_threads);

}

#endif

