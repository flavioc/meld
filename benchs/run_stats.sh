#!/bin/bash

source $PWD/lib/common.sh
MAX="${1}"

if [ -z "$COMPILED" ]; then
   ensure_vm
   for prog in ${*:2}; do
      for x in 1 2 4 8 16 24 32; do
         if [ $x -gt $MAX ]; then
            continue
         fi
         bash stat.sh "../meld -f code/$prog.m" "$prog" "$x"
      done
   done
else
   for prog in ${*:2}; do
      compile_test "$prog"
      for x in 1 2 4 8 16 24 32; do
         if [ $x -gt $MAX ]; then
            continue
         fi
         bash stat.sh "./build/$prog" "$prog" $x
      done
   done
fi
