#!/bin/sh

EXEC="../meld -d"
TEST=${1}
FILE="files/$(basename $TEST .m).test"

NODES=$(sh ./number_nodes.sh $TEST)

do_exit ()
{
	echo $*
	exit 1
}

do_test ()
{
	NTHREADS=${1}
	SCHED=${2}
	TO_RUN="${EXEC} -f ${TEST} -c ${SCHED}${NTHREADS}"

	${TO_RUN} > test.out

	DIFF=`diff -u ${FILE} test.out`
	if [ ! -z "${DIFF}" ]; then
		echo "DIFFERENCES!!!"
		diff -u ${FILE} test.out
		exit 1
	fi
	rm test.out
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
	run_test_n 1 2 ${SCHED}
	run_test_n 2 3 ${SCHED}
	if [ $NODES -gt 2 ]; then
		run_test_n 3 3 ${SCHED}
	fi
	if [ $NODES -gt 3 ]; then
		run_test_n 4 3 ${SCHED}
	fi
}

loop_sched ts
loop_sched tl
loop_sched td
