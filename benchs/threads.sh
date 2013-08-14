#!/bin/bash
# Runs threaded meld and takes execution times
# MELD_ARGS are used to pass arguments to the meld program

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
	
	TO_RUN="${EXEC} ${FILE} -c ${SCHEDULER} -- ${MELD_ARGS}"
	time_run_n "${TO_RUN}" "$(basename ${FILE} .m) ${SCHEDULER}"
}

do_time_sched "${FILE}"
