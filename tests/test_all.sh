#!/bin/bash
WHAT="$1"
RUNS="$2"

EXCEPT_LIST="non_deterministic.exclude"

if [ -z $RUNS ]; then
   RUNS=1
fi

run_test () {
   ./test.sh $1 $WHAT $2
   if [ $? != 0 ]; then
      echo "TEST FAILED"
      return 1
   fi
   return 0
}

not_excluded () {
   [[ -z `grep "^$1$" $EXCEPT_LIST` ]]
}

if [ $WHAT = "serial" ]; then
   # We run all tests.
   for file in `ls code/*.m`; do
      if ! run_test $file $RUNS; then
         exit 1
      fi
   done
else
   # We exclude tests that will give different results depending
   # on thread scheduling.
   for file in `ls code/*.m`; do
      if not_excluded `basename $file .m`; then
         if ! run_test $file $RUNS; then
            exit 1
         fi
      fi
   done
fi
