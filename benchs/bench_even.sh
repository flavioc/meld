#!/bin/bash

SCHEDULER="${1}"
FILE="${2}"

RUN="bash ./benchmark.sh"

do_run ()
{
	NUM_THREADS="${1}"
	$RUN "${SCHEDULER}${NUM_THREADS}" ${FILE}
}

do_run 1
for((I = 2; I <= 16; I += 2)); do
	do_run $I
done
