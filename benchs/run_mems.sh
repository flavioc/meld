#!/bin/bash

source $PWD/lib/common.sh

if [ -z "$COMPILED" ]; then
   ensure_vm
   for prog in ${*:1}; do
      for x in 1 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32; do
         bash mem.sh "../meld -f code/$prog.m" "$prog" $x
      done
   done
else
   for prog in ${*:1}; do
      compile_test "$prog"
      for x in 1 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32; do
         bash mem.sh "./build/$prog" "$prog" $x
      done
   done
fi
