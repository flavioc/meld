#!/bin/bash

SCHEDULER="${1}"
STEP="${2}"
MIN="${3}"
MAX="${4}"
RUNS="${5}"

rm -f $RESULTS_FILE

for x in ${*:6}; do
   bash threads_even.sh code/$x.m $SCHEDULER $STEP $MIN $MAX $RUNS
done
