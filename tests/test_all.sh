#!/bin/bash
WHAT="$1"
RUNS="$2"
EXCEPT_LIST="non_deterministic.exclude" #"$2"

if [ "$WHAT" == "sl" ]; then
   EXCEPT_LIST=""
fi

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

if [ -z "$EXCEPT_LIST" ]; then
   for file in `ls code/*.m`; do
      if ! run_test $file $RUNS; then
         exit 1
      fi
   done
else
   for file in `ls code/*.m`; do
      if not_excluded `basename $file .m`; then
         if ! run_test $file $RUNS; then
            exit 1
         fi
      fi
   done
fi
