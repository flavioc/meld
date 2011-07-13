#!/bin/bash

EXEC="../meld -d"
TEST=${1}
TYPE="${2}"

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

do_test_mix ()
{
	NPROCS=${1}
	NTHREADS=${2}
	MIX_TYPE=${3}

	TO_RUN="mpirun -n ${NPROCS} ${EXEC} -f ${TEST} -c ${MIX_TYPE}${NTHREADS}"

	run_diff "${TO_RUN}"
}

run_serial_n ()
{
	SCHED=${1}
	TIMES=${2}

	echo "Running ${TEST} ${TIMES} times (SCHED: ${SCHED})..."
	for((I=1; I <= ${TIMES}; I++)); do
		do_serial ${SCHED}
	done
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

run_test_mix_n ()
{
	NPROCS=${1}
	NTHREADS=${2}
	TIMES=${3}
	MIX_TYPE=${4}
	echo "Running ${TEST} ${TIMES} times with ${NPROCS} processes and ${NTHREADS} threads (SCHED: ${MIX_TYPE})..."
	for((I=1; I <= ${TIMES}; I++)); do
		do_test_mix ${NPROCS} ${NTHREADS} ${MIX_TYPE}
	done
}

[ -f "${TEST}" ] || do_exit "Test code ${TEST} not found"
[ -f "${FILE}" ] || do_exit "Test file ${FILE} not found"

loop_sched ()
{
	SCHED=${1}
	run_test_n 1 1 ${SCHED}
	run_test_n 2 1 ${SCHED}
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

loop_sched_mix ()
{
	MIX_TYPE=${1}
	run_test_mix_n 1 1 1 ${MIX_TYPE}
	run_test_mix_n 2 1 1 ${MIX_TYPE}
	run_test_mix_n 1 2 1 ${MIX_TYPE}
	
	if [ $NODES -gt 2 ]; then
		run_test_mix_n 3 1 1 ${MIX_TYPE}
		run_test_mix_n 1 3 1 ${MIX_TYPE}
	fi
	if [ $NODES -gt 3 ]; then
		run_test_mix_n 4 1 1 ${MIX_TYPE}
		run_test_mix_n 1 4 1 ${MIX_TYPE}
		run_test_mix_n 2 2 1 ${MIX_TYPE}
	fi
	if [ $NODES -gt 4 ]; then
		run_test_mix_n 5 1 1 ${MIX_TYPE}
		run_test_mix_n 1 5 1 ${MIX_TYPE}
	fi
	if [ $NODES -gt 5 ]; then
		run_test_mix_n 6 1 1 ${MIX_TYPE}
		run_test_mix_n 1 6 1 ${MIX_TYPE}
		run_test_mix_n 2 3 1 ${MIX_TYPE}
		run_test_mix_n 3 2 1 ${MIX_TYPE}
	fi
	if [ $NODES -gt 6 ]; then
		run_test_mix_n 7 1 1 ${MIX_TYPE}
		run_test_mix_n 1 7 1 ${MIX_TYPE}
	fi
	if [ $NODES -gt 7 ]; then
		run_test_mix_n 8 1 1 ${MIX_TYPE}
		run_test_mix_n 1 8 1 ${MIX_TYPE}
		run_test_mix_n 2 4 1 ${MIX_TYPE}
		run_test_mix_n 4 2 1 ${MIX_TYPE}
	fi
}

if [ "${TYPE}" = "all" ]; then
	loop_sched ts
	loop_sched tl
	loop_sched td
	loop_sched_mix mpiglobal
	loop_sched_mix mpistatic
	loop_sched_mix mpidynamic
	loop_sched_mix mpisingle
	exit 0
fi

if [ "${TYPE}" = "serial" ]; then
	run_serial_n sg 1
	run_serial_n sl 1
	exit 0
fi

if [ "${TYPE}" = "ts" ]; then
	loop_sched ts
	exit 0
fi

if [ "${TYPE}" = "tl" ]; then
	loop_sched tl
	exit 0
fi

if [ "${TYPE}" = "td" ]; then
	loop_sched td
	exit 0
fi

if [ "${TYPE}" = "mpiglobal" ]; then
	loop_sched_mix mpiglobal
	exit 0
fi

if [ "${TYPE}" = "mpistatic" ]; then
	loop_sched_mix mpistatic
	exit 0
fi

if [ "${TYPE}" = "mpidynamic" ]; then
	loop_sched_mix mpidynamic
	exit 0
fi

if [ "${TYPE}" = "mpisingle" ]; then
	loop_sched_mix mpisingle
	exit 0
fi
