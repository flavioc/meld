#!/bin/bash
WHAT="$1"
RUNS="$2"

EXCEPT_LIST="non_deterministic.exclude"

source ../benchs/lib/common.sh

if [ -z $RUNS ]; then
   RUNS=1
fi

run_test () {
   BT="$1"
   RUNS="$2"
   if [ -z "$COMPILED" ]; then
      EXEC="../meld -f code/$BT.m"
      ensure_vm
   else
      compile_test "$BT"
      EXEC="build/$BT"
   fi
   ./test.sh "$EXEC" "$BT" "$WHAT" "$RUNS"
   if [ $? != 0 ]; then
      if [ -z "$IGNORE" ]; then
         return 1
      else
         return 0
      fi
   fi
   return 0
}

not_excluded () {
   [[ -z `grep "^$1$" $EXCEPT_LIST` ]]
}

if [ $WHAT = "serial" ]; then
   # We run all tests.
   for file in `ls code/*.m`; do
      if ! run_test $(basename $file .m) $RUNS; then
         exit 1
      fi
   done
else
   # We exclude tests that will give different results depending
   # on thread scheduling.
   for file in `ls code/*.m`; do
      if not_excluded `basename $file .m`; then
         if ! run_test $(basename $file .m) $RUNS; then
            exit 1
         fi
      fi
   done
fi
