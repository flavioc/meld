#!/bin/bash

SCHEDULER="${1}"
STEP="${2}"
MIN="${3}"
MAX="${4}"
RUNS="${5}"

rm -f $RESULTS_FILE
source $PWD/lib/common.sh

if [ -z "$COMPILED" ]; then
   ensure_vm
   for x in ${*:6}; do
      bash threads_even.sh "../meld -f code/$x.m" "$x" $SCHEDULER $STEP $MIN $MAX $RUNS
   done
else
   for x in ${*:6}; do
      compile_test "$x"
      if [ -z "$COMPILE_ONLY" ]; then
         bash threads_even.sh "./build/$x" "$x" $SCHEDULER $STEP $MIN $MAX $RUNS
      fi
   done
fi
