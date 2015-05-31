#!/bin/bash

EXEC=${1}
BT="${2}"
TYPE="${3}"
RUNS="${4}"

if test -z "${BT}" -o -z "${TYPE}"; then
	echo "Usage: test.sh <program> <test file> <test type: th, thp, ...>"
	exit 1
fi

dynamic_test () {
   [[ ! -z `grep "^$1$" "dynamic.include"` ]]
}

to_exclude () {
   [[ ! -z `grep "^$1$" "blacklist.exclude"` ]]
}

FILE="files/$BT.test"
ARGS="args/$BT"
NODES=$(python ./number_nodes.py code/$BT.m)

if [ -f $ARGS ]; then
   . $ARGS
fi

if dynamic_test "$BT"; then
   # We can actually use more than 1 thread.
   NODES=64
fi
if to_exclude "$BT"; then
   exit 0
fi

do_exit ()
{
	echo $*
	exit 1
}

run_diff ()
{
	TO_RUN="${1} ${MELD_ARGS} -- ${PROG_ARGS}"
	${TO_RUN} > test.out
	RET=$?
	if [ $RET -eq 1 ]; then
		echo "Linear Meld has failed! See report"
		exit 1
	fi
   if [ $RET -eq 139 ]; then
      echo "Linear Meld crashed. Please report to <flaviocruz@gmail.com>"
      exit 1
   fi
   failed_tests=""
   done=0
   for file in "${FILE}"*; do
      DIFF=`diff -u ${file} test.out`
      if [ ! -z "${DIFF}" ]; then
         failed_tests="$failed_tests $file"
      else
         done=1
         break
      fi
   done
   if [ $done -eq 0 ]; then 
      for tst in $failed_tests; do
         diff_file="/tmp/test.$$.tmp"
         diff -u ${tst} test.out > $diff_file
         lines=`wc -w $diff_file | cut -d' ' -f1`
         if [ $lines -gt 20 ]; then
            cat $diff_file | tail -n 20
            echo "... (difference is greater than 20 lines)"
         else
            cat $diff_file
         fi
         rm -f $diff_file
         echo "!!!!!! DIFFERENCES IN FILE ${BT} ($TO_RUN)"
         return 1
      done
   fi
	rm test.out
   return 0
}

do_serial ()
{
	SCHED=${1}
	TO_RUN="${EXEC} -d -c ${SCHED}"
	
	run_diff "${TO_RUN}"
}

do_test ()
{
	NTHREADS=${1}
	SCHED=${2}
	TO_RUN="${EXEC} -d -c ${SCHED}${NTHREADS}"

	run_diff "${TO_RUN}"
}

run_serial_n ()
{
	SCHED=${1}
	TIMES=${2}

	echo -n "Running ${BT} ${TIMES} times (SCHED: ${SCHED})..."
	for((I=1; I <= ${TIMES}; I++)); do
		if ! do_serial ${SCHED}; then
         return 1
      fi
	done
   echo " OK!"
   return 0
}

run_test_n ()
{
	NTHREADS=${1}
	TIMES=${2}
	SCHED=${3}
	echo -n "Running ${BT} ${TIMES} times with ${NTHREADS} threads (SCHED: ${SCHED})..."
	for((I=1; I <= ${TIMES}; I++)); do
		do_test ${NTHREADS} ${SCHED}
	done
   echo " OK!"
   return 0
}

[ -f "${FILE}" ] || do_exit "Test file ${FILE} not found"

loop_sched ()
{
	SCHED=${1}
   RUNS=${2}
   # run_test_n 1 ${RUNS} ${SCHED}
   for th in `seq 2 16`; do
      if [ $NODES -ge $th ]; then
         run_test_n $th ${RUNS} ${SCHED}
      fi
   done
}

if [ "${TYPE}" = "thread" ]; then
	loop_sched th ${RUNS}
	exit $?
fi

if [ "${TYPE}" = "serial" ]; then
   run_serial_n th1 ${RUNS}
	exit $?
fi
