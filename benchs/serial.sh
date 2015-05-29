#!/bin/sh

EXEC="${1}"
NAME="${2}"
RUNS="${3}"

if [ -z "${RUNS}" ]; then
	RUNS=1
fi

if [ -z "$EXEC" ]; then
   echo "usage: serial.sh <command> <name> [n-runs=3]"
   exit 1
fi
if [ -z "$NAME" ]; then
   echo "usage: serial.sh <command> <name> [n-runs=3]"
   exit 1
fi

source $PWD/lib/common.sh
TO_RUN="${EXEC} -c th1 -t"
time_run_n "${TO_RUN}" "$NAME" th 1
