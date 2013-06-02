#!/bin/bash

EXEC="../meld -d"
TEST=${1}
TYPE="${2}"
FORCE_THREADS="${3}"

if test -z "${TEST}" -o -z "${TYPE}"; then
	echo "Usage: test.sh <code file> <test type: serial, ts, tl, ...>"
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
		echo "DIFFERENCES!!!"
		diff -u ${FILE} test.out
		exit 1
	fi
	rm test.out
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
		do_serial ${SCHED}
	done
   echo " OK!"
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
	loop_sched tl
	exit 0
fi

if [ "${TYPE}" = "sl" ]; then
	run_serial_n sl 1
	exit 0
fi

if [ "${TYPE}" = "tl" ]; then
	loop_sched tl
	exit 0
fi
