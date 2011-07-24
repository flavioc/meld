#!/bin/bash

SCHEDULER="${1}"
FILE="${2}"
MIN="${3}"
MAX="${4}"

if [ -z "${SCHEDULER}" ] || [ -z "${FILE}" ]; then
	echo "Usage: threads_even.sh <scheduler> <file> [min=2] [max=16]"
	exit 1
fi

if [ -z "${MIN}" ]; then
	MIN=2
fi
if [ -z "${MAX}" ]; then
	MAX=16
fi


RUN="bash ./threads.sh"

do_run ()
{
	NUM_THREADS="${1}"
	$RUN "${SCHEDULER}${NUM_THREADS}" ${FILE}
}

do_run 1
for((I = ${MIN}; I <= ${MAX}; I += 2)); do
	do_run $I
done
