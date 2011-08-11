#!/bin/bash
# Runs meld and takes execution times
# EXTRA_ARGS variable can be used to pass other arguments to the executable.

SCHEDULER="${1}"
PROCS="${2}"
THREADS="${3}"
FILE="${4}"
RUNS="${5}"

if [ -z "${RUNS}" ]; then
	RUNS=3
fi

if [ -z "${SCHEDULER}" ] || [ -z "${FILE}" ] || [ -z "${PROCS}" ] || [ -z "${THREADS}" ]; then
	echo "Usage: mix.sh <scheduler> <procs> <threads> <file> [num runs=3]"
	exit 1
fi

source $PWD/lib/common.sh

do_time_mpi "${FILE}" "${PROCS}" "${THREADS}"
