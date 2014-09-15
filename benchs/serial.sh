#!/bin/sh

FILE="${1}"
RUNS="${2}"

if [ -z "${RUNS}" ]; then
	RUNS=1
fi

if [ -z "$FILE" ]; then
   echo "usage: serial.sh <file> [n-runs=3]"
   exit 1
fi

source $PWD/lib/common.sh
TO_RUN="${EXEC} ${FILE} -c sl -- ${MELD_ARGS}"
time_run_n "${TO_RUN}" "$(basename ${FILE} .m) sl"
