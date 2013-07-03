#!/bin/bash
WHAT="$1"
EXCEPT_LIST="non_deterministic.exclude" #"$2"

if [ "$WHAT" == "sl" ]; then
   EXCEPT_LIST=""
fi

run_test () {
   ./test.sh $1 $WHAT || (echo "TEST FAILED" && exit 1)
}

not_excluded () {
   [[ -z `grep "^$1$" $EXCEPT_LIST` ]]
}

if [ -z "$EXCEPT_LIST" ]; then
   for file in `ls code/*.m`; do
      run_test $file || exit 1
   done
else
   for file in `ls code/*.m`; do
      if not_excluded `basename $file .m`; then
         run_test $file || exit 1
      fi
   done
fi
