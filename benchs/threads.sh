#!/bin/bash
# Runs threaded meld and takes execution times
# EXTRA_ARGS variable can be used to pass other arguments to the executable.

SCHEDULER="${1}"
FILE="${2}"
RUNS="${3}"

if [ -z "${RUNS}" ]; then
	RUNS=3
fi

if [ -z "${SCHEDULER}" ] || [ -z "${FILE}" ]; then
	echo "Usage: threads.sh <scheduler> <file> [num runs=3]"
	exit 1
fi

source $PWD/lib/common.sh

do_time_sched ()
{
	FILE="${1}"
	
	TO_RUN="${EXEC} ${FILE} -c ${SCHEDULER}"
	time_run_n "${TO_RUN}" "$(basename ${FILE} .m) ${SCHEDULER}"
}

do_time_sched "${FILE}"
