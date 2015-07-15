#!/bin/bash

source $PWD/lib/common.sh

if [ -z "$COMPILED" ]; then
   ensure_vm
   for x in ${*:1}; do
      bash stat.sh "../meld -f code/$x.m" "$x"
   done
else
   for x in ${*:1}; do
      compile_test "$x"
      bash stat.sh "./build/$x" "$x" $RUNS
   done
fi
