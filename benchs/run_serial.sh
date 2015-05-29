#!/bin/bash

RUNS="${1}"

rm -f $RESULTS_FILE
source $PWD/lib/common.sh

if [ -z "$COMPILED" ]; then
   ensure_vm
   for x in ${*:2}; do
      bash serial.sh "../meld -f code/$x.m" "$x" $RUNS
   done
else
   for x in ${*:2}; do
      compile_test "$x"
      if [ -z "$COMPILE_ONLY" ]; then
         bash serial.sh "./build/$x" "$x" $RUNS
      fi
   done
fi
