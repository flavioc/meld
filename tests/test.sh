#!/bin/bash

EXEC="../meld -d"
TEST=${1}
TYPE="${2}"
FORCE_THREADS="${3}"

if test -z "${TEST}" -o -z "${TYPE}"; then
	echo "Usage: test.sh <code file> <test type: th, thp, ...>"
	exit 1
fi

FILE="files/$(basename $TEST .m).test"
NODES=$(sh ./number_nodes.sh $TEST)

do_exit ()
{
	echo $*
	exit 1
}

run_diff ()
{
	TO_RUN="${1}"
	${TO_RUN} > test.out
	RET=$?
	if [ $RET -eq 1 ]; then
		echo "Meld failed! See report"
		exit 1
	fi
	DIFF=`diff -u ${FILE} test.out`
	if [ ! -z "${DIFF}" ]; then
		diff -u ${FILE} test.out
		echo "!!!!!! DIFFERENCES IN FILE ${TEST} ($TO_RUN)"
      return 1
	fi
	rm test.out
   return 0
}

do_serial ()
{
	SCHED=${1}
	TO_RUN="${EXEC} -f ${TEST} -c ${SCHED}"
	
	run_diff "${TO_RUN}"
}

do_test ()
{
	NTHREADS=${1}
	SCHED=${2}
	TO_RUN="${EXEC} -f ${TEST} -c ${SCHED}${NTHREADS}"

	run_diff "${TO_RUN}"
}

run_serial_n ()
{
	SCHED=${1}
	TIMES=${2}

	echo -n "Running ${TEST} ${TIMES} times (SCHED: ${SCHED})..."
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
	echo "Running ${TEST} ${TIMES} times with ${NTHREADS} threads (SCHED: ${SCHED})..."
	for((I=1; I <= ${TIMES}; I++)); do
		do_test ${NTHREADS} ${SCHED}
	done
}

[ -f "${TEST}" ] || do_exit "Test code ${TEST} not found"
[ -f "${FILE}" ] || do_exit "Test file ${FILE} not found"

loop_sched ()
{
	SCHED=${1}
   if [ ! -z "$FORCE_THREADS" ]; then
      run_test_n $FORCE_THREADS 1 ${SCHED}
      return
   fi
	run_test_n 1 1 ${SCHED}
   if [ $NODES -gt 1 ]; then
      run_test_n 2 1 ${SCHED}
   fi
	if [ $NODES -gt 2 ]; then
		run_test_n 3 1 ${SCHED}
	fi
	if [ $NODES -gt 3 ]; then
		run_test_n 4 1 ${SCHED}
	fi
	if [ $NODES -gt 4 ]; then
		run_test_n 5 1 ${SCHED}
	fi
	if [ $NODES -gt 5 ]; then
		run_test_n 6 1 ${SCHED}
	fi
	if [ $NODES -gt 6 ]; then
		run_test_n 7 1 ${SCHED}
	fi
	if [ $NODES -gt 7 ]; then
		run_test_n 8 1 ${SCHED}
	fi
}

if [ "${TYPE}" = "all" ]; then
	loop_sched th
	exit 0
fi

if [ "${TYPE}" = "th" ]; then
   run_serial_n th1 1
	exit 0
fi
