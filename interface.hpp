
#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include "vm/state.hpp"
#include "machine.hpp"

extern size_t num_threads;
extern bool show_database;
extern bool dump_database;
extern bool time_execution;
extern bool scheduling_mechanism;
extern bool work_stealing;

void parse_sched(char *);
void help_schedulers(void);
bool run_program(process::machine&);

#endif
