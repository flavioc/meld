#!/bin/bash
# Runs meld and takes execution times
# EXTRA_ARGS variable can be used to pass other arguments to the executable.

SCHEDULER="${1}"
PROCS="${2}"
FILE="${3}"
RUNS="${4}"

if [ -z "${RUNS}" ]; then
	RUNS=3
fi

if [ -z "${SCHEDULER}" ] || [ -z "${FILE}" ] || [ -z "${PROCS}" ]; then
	echo "Usage: mpi.sh <scheduler> <procs> <file> [num runs=3]"
	exit 1
fi

source $PWD/lib/common.sh

do_time_mpi "${FILE}" "${PROCS}" 1
