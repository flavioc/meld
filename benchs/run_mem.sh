#!/bin/bash

source $PWD/lib/common.sh

if [ -z "$COMPILED" ]; then
   ensure_vm
   for x in ${*:1}; do
      bash mem.sh "../meld -f code/$x.m" "$x"
   done
else
   for x in ${*:1}; do
      compile_test "$x"
      bash mem.sh "./build/$x" "$x"
   done
fi
