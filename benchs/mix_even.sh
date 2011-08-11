#!/bin/bash

SCHEDULER="${1}"
FILE="${2}"

if [ -z "${SCHEDULER}" ] || [ -z "${FILE}" ]; then
	echo "Usage: mix_even.sh <scheduler> <file>"
	exit 1
fi

RUN="bash ./mix.sh"

do_run ()
{
	NUM_PROCS="${1}"
	NUM_THREADS="${2}"
	$RUN "${SCHEDULER}" "${NUM_PROCS}" "${NUM_THREADS}" ${FILE}
}

do_run 2 2
do_run 2 3
do_run 3 2
do_run 2 4
do_run 4 2
do_run 3 4
do_run 4 3
do_run 4 4
do_run 2 8
do_run 8 2
do_run 2 5
do_run 5 2
do_run 2 6
do_run 6 2
do_run 2 7
do_run 7 2
